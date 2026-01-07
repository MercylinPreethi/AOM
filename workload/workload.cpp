// workload.cpp - AOM AV1 Encoder Workload with Timing and Logging
// Encodes raw YUV video to AV1 format using libaom

#include <iostream>
#include <fstream>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <memory>

// AOM encoder headers
#include "aom/aom_encoder.h"
#include "aom/aomcx.h"
#include <string> 

// Configuration
struct Config {
    const char* input_file;
    const char* output_file;
    int width;
    int height;
    int fps;
    int frames;
    int bitrate;  // kbps
    int cpu_used; // 0-8, higher = faster but lower quality
    int threads;
};

// Timing helper
class Timer {
    std::chrono::high_resolution_clock::time_point start;
public:
    Timer() : start(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed_ms() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
    
    void reset() {
        start = std::chrono::high_resolution_clock::now();
    }
};

// Logger
class Logger {
public:
    static void info(const std::string& msg) {
        std::cout << "[INFO] " << msg << std::endl;
    }
    
    static void error(const std::string& msg) {
        std::cerr << "[ERROR] " << msg << std::endl;
    }
    
    static void timing(const std::string& msg, double ms) {
        std::cout << "[TIMING] " << msg << ": " 
                  << std::fixed << std::setprecision(2) << ms << " ms" << std::endl;
    }
    
    static void stats(const std::string& msg) {
        std::cout << "[STATS] " << msg << std::endl;
    }
};

// AOM Encoder wrapper
class AOMEncoder {
private:
    aom_codec_ctx_t codec;
    aom_codec_enc_cfg_t cfg;
    aom_image_t* image;
    bool initialized;
    
    int frame_count;
    size_t total_bytes;
    double total_encode_time_ms;
    
public:
    AOMEncoder() : image(nullptr), initialized(false), 
                   frame_count(0), total_bytes(0), total_encode_time_ms(0.0) {}
    
    ~AOMEncoder() {
        if (initialized) {
            aom_codec_destroy(&codec);
        }
        if (image) {
            aom_img_free(image);
        }
    }
    
    bool init(const Config& config) {
        Logger::info("Initializing AOM encoder...");
        
        // Get default encoder configuration
        aom_codec_err_t res = aom_codec_enc_config_default(aom_codec_av1_cx(), &cfg, 0);
        if (res != AOM_CODEC_OK) {
            Logger::error("Failed to get default encoder config");
            return false;
        }
        
        // Configure encoder
        cfg.g_w = config.width;
        cfg.g_h = config.height;
        cfg.g_timebase.num = 1;
        cfg.g_timebase.den = config.fps;
        cfg.rc_target_bitrate = config.bitrate;
        cfg.g_error_resilient = 0;
        cfg.g_threads = config.threads;
        cfg.g_lag_in_frames = 0; // No lookahead for faster encoding
        
        Logger::info("Encoder configuration:");
        Logger::info("  Resolution: " + std::to_string(config.width) + "x" + std::to_string(config.height));
        Logger::info("  FPS: " + std::to_string(config.fps));
        Logger::info("  Bitrate: " + std::to_string(config.bitrate) + " kbps");
        Logger::info("  Threads: " + std::to_string(config.threads));
        Logger::info("  CPU Used: " + std::to_string(config.cpu_used));
        
        // Initialize encoder
        res = aom_codec_enc_init(&codec, aom_codec_av1_cx(), &cfg, 0);
        if (res != AOM_CODEC_OK) {
            Logger::error("Failed to initialize encoder: " + std::string(aom_codec_error(&codec)));
            return false;
        }
        
        // Set cpu-used parameter (encoding speed)
        res = aom_codec_control(&codec, AOME_SET_CPUUSED, config.cpu_used);
        if (res != AOM_CODEC_OK) {
            Logger::error("Failed to set cpu-used: " + std::string(aom_codec_error(&codec)));
            return false;
        }
        
        // Allocate image
        image = aom_img_alloc(nullptr, AOM_IMG_FMT_I420, config.width, config.height, 1);
        if (!image) {
            Logger::error("Failed to allocate image");
            return false;
        }
        
        initialized = true;
        Logger::info("Encoder initialized successfully");
        return true;
    }
    
    bool encode_frame(const uint8_t* yuv_data, std::ofstream& outfile, int frame_num) {
        Timer timer;
        
        // Copy YUV data to image
        size_t y_size = image->stride[0] * cfg.g_h;
        size_t u_size = image->stride[1] * ((cfg.g_h + 1) / 2);
        size_t v_size = image->stride[2] * ((cfg.g_h + 1) / 2);
        
        memcpy(image->planes[0], yuv_data, y_size);
        memcpy(image->planes[1], yuv_data + y_size, u_size);
        memcpy(image->planes[2], yuv_data + y_size + u_size, v_size);
        
        // Encode frame
        aom_codec_err_t res = aom_codec_encode(&codec, image, frame_num, 1, 0);
        if (res != AOM_CODEC_OK) {
            Logger::error("Failed to encode frame " + std::to_string(frame_num) + 
                         ": " + std::string(aom_codec_error(&codec)));
            return false;
        }
        
        // Get encoded data
        aom_codec_iter_t iter = nullptr;
        const aom_codec_cx_pkt_t* pkt;
        size_t frame_bytes = 0;
        
        while ((pkt = aom_codec_get_cx_data(&codec, &iter)) != nullptr) {
            if (pkt->kind == AOM_CODEC_CX_FRAME_PKT) {
                outfile.write(static_cast<const char*>(pkt->data.frame.buf), 
                             pkt->data.frame.sz);
                frame_bytes += pkt->data.frame.sz;
                total_bytes += pkt->data.frame.sz;
            }
        }
        
        double encode_time = timer.elapsed_ms();
        total_encode_time_ms += encode_time;
        frame_count++;
        
        if (frame_num % 30 == 0) { // Log every 30 frames
            Logger::stats("Frame " + std::to_string(frame_num) + 
                         " | Size: " + std::to_string(frame_bytes) + " bytes" +
                         " | Time: " + std::to_string(encode_time) + " ms");
        }
        
        return true;
    }
    
    bool flush(std::ofstream& outfile) {
        Logger::info("Flushing encoder...");
        
        // Flush encoder
        aom_codec_err_t res = aom_codec_encode(&codec, nullptr, 0, 0, 0);
        if (res != AOM_CODEC_OK) {
            Logger::error("Failed to flush encoder: " + std::string(aom_codec_error(&codec)));
            return false;
        }
        
        // Get remaining packets
        aom_codec_iter_t iter = nullptr;
        const aom_codec_cx_pkt_t* pkt;
        
        while ((pkt = aom_codec_get_cx_data(&codec, &iter)) != nullptr) {
            if (pkt->kind == AOM_CODEC_CX_FRAME_PKT) {
                outfile.write(static_cast<const char*>(pkt->data.frame.buf), 
                             pkt->data.frame.sz);
                total_bytes += pkt->data.frame.sz;
            }
        }
        
        return true;
    }
    
    void print_statistics() {
        Logger::info("===========================================");
        Logger::info("Encoding Statistics");
        Logger::info("===========================================");
        Logger::stats("Total frames encoded: " + std::to_string(frame_count));
        Logger::stats("Total output size: " + std::to_string(total_bytes) + " bytes" +
                     " (" + std::to_string(total_bytes / 1024.0) + " KB)");
        Logger::stats("Average frame size: " + std::to_string(total_bytes / (double)frame_count) + " bytes");
        Logger::stats("Total encoding time: " + std::to_string(total_encode_time_ms) + " ms");
        Logger::stats("Average frame time: " + std::to_string(total_encode_time_ms / frame_count) + " ms");
        Logger::stats("Encoding FPS: " + std::to_string(frame_count / (total_encode_time_ms / 1000.0)));
        Logger::info("===========================================");
    }
};

// Main workload
int main(int argc, char* argv[]) {
    Logger::info("AOM AV1 Encoder Workload");
    Logger::info("===========================================");
    
    // Default configuration
    Config config = {
        .input_file = "input.yuv",
        .output_file = "output.ivf",
        .width = 1920,
        .height = 1080,
        .fps = 30,
        .frames = 300,
        .bitrate = 2000,  // 2 Mbps
        .cpu_used = 6,    // Fast encoding
        .threads = 4
    };
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            config.input_file = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            config.output_file = argv[++i];
        } else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            config.width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            config.height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-fps") == 0 && i + 1 < argc) {
            config.fps = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-frames") == 0 && i + 1 < argc) {
            config.frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            config.bitrate = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-cpu") == 0 && i + 1 < argc) {
            config.cpu_used = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            config.threads = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0) {
            std::cout << "Usage: workload [options]\n"
                      << "Options:\n"
                      << "  -i <file>     Input YUV file (default: input.yuv)\n"
                      << "  -o <file>     Output IVF file (default: output.ivf)\n"
                      << "  -w <width>    Video width (default: 1920)\n"
                      << "  -h <height>   Video height (default: 1080)\n"
                      << "  -fps <fps>    Frame rate (default: 30)\n"
                      << "  -frames <n>   Number of frames (default: 300)\n"
                      << "  -b <bitrate>  Bitrate in kbps (default: 2000)\n"
                      << "  -cpu <0-8>    CPU used (default: 6, higher=faster)\n"
                      << "  -t <threads>  Number of threads (default: 4)\n"
                      << "  --help        Show this help\n";
            return 0;
        }
    }
    
    Logger::info("Input: " + std::string(config.input_file));
    Logger::info("Output: " + std::string(config.output_file));
    
    Timer total_timer;
    
    // Open input file
    std::ifstream infile(config.input_file, std::ios::binary);
    if (!infile) {
        Logger::error("Cannot open input file: " + std::string(config.input_file));
        return 1;
    }
    
    // Open output file
    std::ofstream outfile(config.output_file, std::ios::binary);
    if (!outfile) {
        Logger::error("Cannot create output file: " + std::string(config.output_file));
        return 1;
    }
    
    // Initialize encoder
    AOMEncoder encoder;
    if (!encoder.init(config)) {
        Logger::error("Failed to initialize encoder");
        return 1;
    }
    
    // Calculate frame size (I420 format: Y + U/4 + V/4)
    size_t frame_size = config.width * config.height * 3 / 2;
    std::unique_ptr<uint8_t[]> frame_buffer(new uint8_t[frame_size]);
    
    Logger::info("===========================================");
    Logger::info("Starting encoding...");
    Logger::info("Frame size: " + std::to_string(frame_size) + " bytes");
    Logger::info("===========================================");
    
    // Encode frames
    int frame_num = 0;
    while (frame_num < config.frames && infile.read(reinterpret_cast<char*>(frame_buffer.get()), frame_size)) {
        if (!encoder.encode_frame(frame_buffer.get(), outfile, frame_num)) {
            Logger::error("Encoding failed at frame " + std::to_string(frame_num));
            return 1;
        }
        frame_num++;
    }
    
    Logger::info("Processed " + std::to_string(frame_num) + " frames");
    
    // Flush encoder
    if (!encoder.flush(outfile)) {
        Logger::error("Failed to flush encoder");
        return 1;
    }
    
    double total_time = total_timer.elapsed_ms();
    
    // Print statistics
    encoder.print_statistics();
    Logger::timing("Total wall time", total_time);
    Logger::info("Encoding completed successfully!");
    
    return 0;
}