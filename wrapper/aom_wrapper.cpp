// aom_wrapper.cpp - Implementation of AOM Wrapper Library
#include "aom_wrapper.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <regex>
#include <vector>
#include <thread>

#include "aom/aom_encoder.h"
#include "aom/aom_decoder.h"
#include "aom/aomcx.h"
#include "aom/aomdx.h"

// ============================================================================
// Helper Functions
// ============================================================================

static void write_u16(std::ofstream& file, uint16_t val) {
    uint8_t bytes[2] = {
        static_cast<uint8_t>(val & 0xff),
        static_cast<uint8_t>((val >> 8) & 0xff)
    };
    file.write(reinterpret_cast<char*>(bytes), 2);
}

static void write_u32(std::ofstream& file, uint32_t val) {
    uint8_t bytes[4] = {
        static_cast<uint8_t>(val & 0xff),
        static_cast<uint8_t>((val >> 8) & 0xff),
        static_cast<uint8_t>((val >> 16) & 0xff),
        static_cast<uint8_t>((val >> 24) & 0xff)
    };
    file.write(reinterpret_cast<char*>(bytes), 4);
}

static void write_u64(std::ofstream& file, uint64_t val) {
    uint8_t bytes[8];
    for (int i = 0; i < 8; i++) {
        bytes[i] = static_cast<uint8_t>((val >> (i * 8)) & 0xff);
    }
    file.write(reinterpret_cast<char*>(bytes), 8);
}

static uint16_t read_le16(const uint8_t* data) {
    return static_cast<uint16_t>(data[0]) | 
           (static_cast<uint16_t>(data[1]) << 8);
}

static uint32_t read_le32(const uint8_t* data) {
    return static_cast<uint32_t>(data[0]) | 
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

static uint64_t read_le64(const uint8_t* data) {
    return static_cast<uint64_t>(data[0]) |
           (static_cast<uint64_t>(data[1]) << 8) |
           (static_cast<uint64_t>(data[2]) << 16) |
           (static_cast<uint64_t>(data[3]) << 24) |
           (static_cast<uint64_t>(data[4]) << 32) |
           (static_cast<uint64_t>(data[5]) << 40) |
           (static_cast<uint64_t>(data[6]) << 48) |
           (static_cast<uint64_t>(data[7]) << 56);
}

// ============================================================================
// Error Messages
// ============================================================================

const char* aom_wrapper_error_string(int result) {
    switch (result) {
        case AOM_WRAPPER_OK:
            return "Success";
        case AOM_WRAPPER_ERROR_INPUT_FILE:
            return "Cannot open input file";
        case AOM_WRAPPER_ERROR_OUTPUT_FILE:
            return "Cannot create output file";
        case AOM_WRAPPER_ERROR_ENCODER_INIT:
            return "Failed to initialize encoder";
        case AOM_WRAPPER_ERROR_DECODER_INIT:
            return "Failed to initialize decoder";
        case AOM_WRAPPER_ERROR_ENCODE_FAILED:
            return "Encoding failed";
        case AOM_WRAPPER_ERROR_DECODE_FAILED:
            return "Decoding failed";
        case AOM_WRAPPER_ERROR_INVALID_PARAMS:
            return "Invalid parameters";
        case AOM_WRAPPER_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        default:
            return "Unknown error";
    }
}

const char* aom_wrapper_version(void) {
    return "1.0.0";
}

// ============================================================================
// Resolution Detection
// ============================================================================

int aom_detect_resolution(const char* filename, int* width, int* height) {
    if (!filename || !width || !height) return 0;
    
    std::string name(filename);
    
    // Pattern: 1920x1080, 3840x2160, etc.
    std::regex pattern(R"((\d{3,4})x(\d{3,4}))", std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(name, match, pattern)) {
        *width = std::stoi(match[1].str());
        *height = std::stoi(match[2].str());
        return 1;
    }
    
    return 0;
}

// ============================================================================
// Default Parameters
// ============================================================================

void aom_encode_params_default(AomEncodeParams* params) {
    if (!params) return;
    
    params->width = 0;      
    params->height = 0;    
    params->fps = 30;
    params->frames = 0;    
    params->bitrate = 2000;
    params->cpu_used = 6;
    params->threads = 0;   
    params->verbose = 1;
}

void aom_decode_params_default(AomDecodeParams* params) {
    if (!params) return;
    
    params->frames = 0;     
    params->verbose = 1;
}

// ============================================================================
// Encoding Implementation
// ============================================================================

int aom_encode(const char* input_yuv_path, const char* output_ivf_path, 
               const AomEncodeParams* params) {
    
    // Use default params if not provided
    AomEncodeParams default_params;
    if (!params) {
        aom_encode_params_default(&default_params);
        params = &default_params;
    }
    
    // Determine resolution
    int width = params->width;
    int height = params->height;
    
    if (width == 0 || height == 0) {
        if (!aom_detect_resolution(input_yuv_path, &width, &height)) {
            width = 1920;
            height = 1080;
            if (params->verbose) {
                std::cout << "[INFO] Using default resolution: " 
                          << width << "x" << height << std::endl;
            }
        } else if (params->verbose) {
            std::cout << "[INFO] Detected resolution: " 
                      << width << "x" << height << std::endl;
        }
    }
    
    // Determine thread count
    int threads = params->threads;
    if (threads == 0) {
        threads = std::thread::hardware_concurrency();
        if (threads == 0) threads = 4;
        if (threads > 8) threads = 8;  
    }
    
    if (params->verbose) {
        std::cout << "[INFO] AOM Encoder Starting" << std::endl;
        std::cout << "[INFO] Input: " << input_yuv_path << std::endl;
        std::cout << "[INFO] Output: " << output_ivf_path << std::endl;
        std::cout << "[INFO] Resolution: " << width << "x" << height << std::endl;
        std::cout << "[INFO] FPS: " << params->fps << std::endl;
        std::cout << "[INFO] Threads: " << threads << std::endl;
    }
    
    // Open input file
    std::ifstream infile(input_yuv_path, std::ios::binary);
    if (!infile) {
        return AOM_WRAPPER_ERROR_INPUT_FILE;
    }
    
    // Initialize encoder
    aom_codec_ctx_t encoder;
    aom_codec_enc_cfg_t enc_cfg;
    
    if (aom_codec_enc_config_default(aom_codec_av1_cx(), &enc_cfg, 0) != AOM_CODEC_OK) {
        return AOM_WRAPPER_ERROR_ENCODER_INIT;
    }
    
    enc_cfg.g_w = width;
    enc_cfg.g_h = height;
    enc_cfg.g_timebase.num = 1;
    enc_cfg.g_timebase.den = params->fps;
    enc_cfg.rc_target_bitrate = params->bitrate;
    enc_cfg.g_threads = threads;
    enc_cfg.g_lag_in_frames = 0;
    
    if (aom_codec_enc_init(&encoder, aom_codec_av1_cx(), &enc_cfg, 0) != AOM_CODEC_OK) {
        return AOM_WRAPPER_ERROR_ENCODER_INIT;
    }
    
    if (aom_codec_control(&encoder, AOME_SET_CPUUSED, params->cpu_used) != AOM_CODEC_OK) {
        aom_codec_destroy(&encoder);
        return AOM_WRAPPER_ERROR_ENCODER_INIT;
    }
    
    // Allocate image
    aom_image_t* image = aom_img_alloc(nullptr, AOM_IMG_FMT_I420, width, height, 1);
    if (!image) {
        aom_codec_destroy(&encoder);
        return AOM_WRAPPER_ERROR_OUT_OF_MEMORY;
    }
    
    // Open IVF output
    std::ofstream outfile(output_ivf_path, std::ios::binary);
    if (!outfile) {
        aom_img_free(image);
        aom_codec_destroy(&encoder);
        return AOM_WRAPPER_ERROR_OUTPUT_FILE;
    }
    
    // Write IVF header
    outfile.write("DKIF", 4);
    write_u16(outfile, 0);
    write_u16(outfile, 32);
    outfile.write("AV01", 4);
    write_u16(outfile, static_cast<uint16_t>(width));
    write_u16(outfile, static_cast<uint16_t>(height));
    write_u32(outfile, static_cast<uint32_t>(params->fps));
    write_u32(outfile, 1);
    
    std::streampos frame_count_pos = outfile.tellp();
    write_u32(outfile, 0); 
    write_u32(outfile, 0);
    
    // Encode frames
    size_t frame_size = width * height * 3 / 2;
    std::vector<uint8_t> buffer(frame_size);
    
    int frame_num = 0;
    uint32_t encoded_frames = 0;
    
    while ((params->frames == 0 || frame_num < params->frames) && 
           infile.read(reinterpret_cast<char*>(buffer.data()), frame_size)) {
       
        size_t y_size = image->stride[0] * height;
        size_t u_size = image->stride[1] * ((height + 1) / 2);
        
        memcpy(image->planes[0], buffer.data(), y_size);
        memcpy(image->planes[1], buffer.data() + y_size, u_size);
        memcpy(image->planes[2], buffer.data() + y_size + u_size, u_size);
        
        // Encode
        if (aom_codec_encode(&encoder, image, frame_num, 1, 0) != AOM_CODEC_OK) {
            aom_img_free(image);
            aom_codec_destroy(&encoder);
            return AOM_WRAPPER_ERROR_ENCODE_FAILED;
        }
        
        // Get packets
        aom_codec_iter_t iter = nullptr;
        const aom_codec_cx_pkt_t* pkt;
        
        while ((pkt = aom_codec_get_cx_data(&encoder, &iter))) {
            if (pkt->kind == AOM_CODEC_CX_FRAME_PKT) {
                write_u32(outfile, static_cast<uint32_t>(pkt->data.frame.sz));
                write_u64(outfile, frame_num);
                outfile.write(static_cast<const char*>(pkt->data.frame.buf), 
                            pkt->data.frame.sz);
                encoded_frames++;
            }
        }
        
        if (params->verbose >= 2 && frame_num % 30 == 0) {
            std::cout << "[INFO] Encoded frame " << frame_num << std::endl;
        }
        
        frame_num++;
    }
    
    // Flush
    aom_codec_encode(&encoder, nullptr, 0, 0, 0);
    
    aom_codec_iter_t iter = nullptr;
    const aom_codec_cx_pkt_t* pkt;
    
    while ((pkt = aom_codec_get_cx_data(&encoder, &iter))) {
        if (pkt->kind == AOM_CODEC_CX_FRAME_PKT) {
            write_u32(outfile, static_cast<uint32_t>(pkt->data.frame.sz));
            write_u64(outfile, encoded_frames);
            outfile.write(static_cast<const char*>(pkt->data.frame.buf), 
                        pkt->data.frame.sz);
            encoded_frames++;
        }
    }
    
    // Update frame count in header
    std::streampos end_pos = outfile.tellp();
    outfile.seekp(frame_count_pos);
    write_u32(outfile, encoded_frames);
    outfile.seekp(end_pos);
    
    // Cleanup
    outfile.close();
    aom_img_free(image);
    aom_codec_destroy(&encoder);
    
    if (params->verbose) {
        std::cout << "[INFO] Encoding completed: " << encoded_frames 
                  << " frames" << std::endl;
    }
    
    return AOM_WRAPPER_OK;
}

int aom_encode_simple(const char* input_yuv_path, const char* output_ivf_path) {
    return aom_encode(input_yuv_path, output_ivf_path, nullptr);
}

// ============================================================================
// Decoding Implementation
// ============================================================================

int aom_decode(const char* input_ivf_path, const char* output_y4m_path, 
               const AomDecodeParams* params) {

    AomDecodeParams default_params;
    if (!params) {
        aom_decode_params_default(&default_params);
        params = &default_params;
    }
    
    if (params->verbose) {
        std::cout << "[INFO] AOM Decoder Starting" << std::endl;
        std::cout << "[INFO] Input: " << input_ivf_path << std::endl;
        std::cout << "[INFO] Output: " << output_y4m_path << std::endl;
    }
    
    // Read IVF file
    std::ifstream infile(input_ivf_path, std::ios::binary | std::ios::ate);
    if (!infile) {
        return AOM_WRAPPER_ERROR_INPUT_FILE;
    }
    
    size_t file_size = infile.tellg();
    infile.seekg(0);
    
    std::vector<uint8_t> file_data(file_size);
    infile.read(reinterpret_cast<char*>(file_data.data()), file_size);
    infile.close();
    
    const uint8_t* data = file_data.data();
    
    // Parse IVF header
    if (memcmp(data, "DKIF", 4) != 0) {
        return AOM_WRAPPER_ERROR_INPUT_FILE;
    }
    
    uint16_t width = read_le16(data + 12);
    uint16_t height = read_le16(data + 14);
    
    if (params->verbose) {
        std::cout << "[INFO] Resolution: " << width << "x" << height << std::endl;
    }
    
    // Initialize decoder
    aom_codec_ctx_t decoder;
    if (aom_codec_dec_init(&decoder, aom_codec_av1_dx(), nullptr, 0) != AOM_CODEC_OK) {
        return AOM_WRAPPER_ERROR_DECODER_INIT;
    }
    
    // Open Y4M output
    std::ofstream outfile(output_y4m_path, std::ios::binary);
    if (!outfile) {
        aom_codec_destroy(&decoder);
        return AOM_WRAPPER_ERROR_OUTPUT_FILE;
    }
    
    bool header_written = false;
    size_t pos = 32; 
    int decoded_frames = 0;
    
    while (pos + 12 <= file_data.size() && 
           (params->frames == 0 || decoded_frames < params->frames)) {
        
        uint32_t frame_size = read_le32(data + pos);
        pos += 12;
        
        if (frame_size == 0 || pos + frame_size > file_data.size()) break;

        if (aom_codec_decode(&decoder, data + pos, frame_size, nullptr) != AOM_CODEC_OK) {
            aom_codec_destroy(&decoder);
            return AOM_WRAPPER_ERROR_DECODE_FAILED;
        }

        aom_codec_iter_t iter = nullptr;
        aom_image_t* img;
        
        while ((img = aom_codec_get_frame(&decoder, &iter))) {

            if (!header_written) {
                std::string header = "YUV4MPEG2 W" + std::to_string(img->d_w) +
                                   " H" + std::to_string(img->d_h) +
                                   " F30:1 Ip A0:0 C420jpeg\n";
                outfile.write(header.c_str(), header.size());
                header_written = true;
            }
            
            // Write frame
            outfile.write("FRAME\n", 6);
            
            for (unsigned int i = 0; i < img->d_h; i++) {
                outfile.write(reinterpret_cast<const char*>(
                    img->planes[0] + i * img->stride[0]), img->d_w);
            }
            for (unsigned int i = 0; i < (img->d_h + 1) / 2; i++) {
                outfile.write(reinterpret_cast<const char*>(
                    img->planes[1] + i * img->stride[1]), (img->d_w + 1) / 2);
            }
            for (unsigned int i = 0; i < (img->d_h + 1) / 2; i++) {
                outfile.write(reinterpret_cast<const char*>(
                    img->planes[2] + i * img->stride[2]), (img->d_w + 1) / 2);
            }
            
            decoded_frames++;
            
            if (params->verbose >= 2 && decoded_frames % 30 == 0) {
                std::cout << "[INFO] Decoded frame " << decoded_frames << std::endl;
            }
        }
        
        pos += frame_size;
    }
    
    // Cleanup
    outfile.close();
    aom_codec_destroy(&decoder);
    
    if (params->verbose) {
        std::cout << "[INFO] Decoding completed: " << decoded_frames 
                  << " frames" << std::endl;
    }
    
    return AOM_WRAPPER_OK;
}

int aom_decode_simple(const char* input_ivf_path, const char* output_y4m_path) {
    return aom_decode(input_ivf_path, output_y4m_path, nullptr);
}

// ============================================================================
// Round-trip
// ============================================================================

int aom_roundtrip_simple(const char* input_yuv_path, const char* output_y4m_path) {
    const char* temp_ivf = "temp_roundtrip.ivf";
    
    int result = aom_encode_simple(input_yuv_path, temp_ivf);
    if (result != AOM_WRAPPER_OK) {
        return result;
    }
    
    result = aom_decode_simple(temp_ivf, output_y4m_path);
    
    // Clean up temp file
    std::remove(temp_ivf);
    
    return result;
}