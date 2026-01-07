// workload.cpp - AOM AV1 Encoder + Decoder Workload
// Encodes raw YUV (I420) to AV1 and decodes it back for verification

#include <iostream>
#include <fstream>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <memory>
#include <string>

// AOM encoder / decoder headers
#include "aom/aom_encoder.h"
#include "aom/aomcx.h"
#include "aom/aom_decoder.h"
#include "aom/aomdx.h"

// ------------------------------------------------------------
// Configuration
// ------------------------------------------------------------
struct Config {
    const char* input_file;
    const char* output_file;
    int width;
    int height;
    int fps;
    int frames;
    int bitrate;   // kbps
    int cpu_used;  // 0-8
    int threads;
};

// ------------------------------------------------------------
// Timer
// ------------------------------------------------------------
class Timer {
    std::chrono::high_resolution_clock::time_point start;
public:
    Timer() : start(std::chrono::high_resolution_clock::now()) {}
    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
};

// ------------------------------------------------------------
// Logger
// ------------------------------------------------------------
class Logger {
public:
    static void info(const std::string& msg) {
        std::cout << "[INFO] " << msg << std::endl;
    }
    static void error(const std::string& msg) {
        std::cerr << "[ERROR] " << msg << std::endl;
    }
    static void stats(const std::string& msg) {
        std::cout << "[STATS] " << msg << std::endl;
    }
    static void timing(const std::string& msg, double ms) {
        std::cout << "[TIMING] " << msg << ": "
                  << std::fixed << std::setprecision(2)
                  << ms << " ms" << std::endl;
    }
};

// ------------------------------------------------------------
// Encoder + Decoder wrapper
// ------------------------------------------------------------
class AOMCodec {
private:
    // Encoder
    aom_codec_ctx_t encoder;
    aom_codec_enc_cfg_t enc_cfg;
    aom_image_t* image;
    bool encoder_initialized;

    // Decoder
    aom_codec_ctx_t decoder;
    bool decoder_initialized;

    // Stats
    int encoded_frames;
    int decoded_frames;
    size_t total_bytes;
    double total_encode_time_ms;

public:
    AOMCodec()
        : image(nullptr),
          encoder_initialized(false),
          decoder_initialized(false),
          encoded_frames(0),
          decoded_frames(0),
          total_bytes(0),
          total_encode_time_ms(0.0) {}

    ~AOMCodec() {
        if (encoder_initialized) {
            aom_codec_destroy(&encoder);
        }
        if (decoder_initialized) {
            aom_codec_destroy(&decoder);
        }
        if (image) {
            aom_img_free(image);
        }
    }

    bool init(const Config& cfg) {
        Logger::info("Initializing AOM encoder...");

        if (aom_codec_enc_config_default(aom_codec_av1_cx(),
                                         &enc_cfg, 0) != AOM_CODEC_OK) {
            Logger::error("Failed to get default encoder config");
            return false;
        }

        enc_cfg.g_w = cfg.width;
        enc_cfg.g_h = cfg.height;
        enc_cfg.g_timebase.num = 1;
        enc_cfg.g_timebase.den = cfg.fps;
        enc_cfg.rc_target_bitrate = cfg.bitrate;
        enc_cfg.g_threads = cfg.threads;
        enc_cfg.g_lag_in_frames = 0; // low latency

        if (aom_codec_enc_init(&encoder, aom_codec_av1_cx(),
                               &enc_cfg, 0) != AOM_CODEC_OK) {
            Logger::error("Failed to initialize encoder");
            return false;
        }

        if (aom_codec_control(&encoder, AOME_SET_CPUUSED,
                              cfg.cpu_used) != AOM_CODEC_OK) {
            Logger::error("Failed to set cpu-used");
            return false;
        }

        image = aom_img_alloc(nullptr, AOM_IMG_FMT_I420,
                              cfg.width, cfg.height, 1);
        if (!image) {
            Logger::error("Failed to allocate image");
            return false;
        }

        encoder_initialized = true;
        Logger::info("Encoder initialized successfully");

        Logger::info("Initializing AOM decoder...");
        if (aom_codec_dec_init(&decoder,
                               aom_codec_av1_dx(),
                               nullptr, 0) != AOM_CODEC_OK) {
            Logger::error("Failed to initialize decoder");
            return false;
        }

        decoder_initialized = true;
        Logger::info("Decoder initialized successfully");
        return true;
    }

    bool encode_and_decode(const uint8_t* yuv,
                           std::ofstream& outfile,
                           int frame_num) {
        Timer t;

        // Copy YUV planes
        size_t y_size = image->stride[0] * enc_cfg.g_h;
        size_t u_size = image->stride[1] * ((enc_cfg.g_h + 1) / 2);
        size_t v_size = image->stride[2] * ((enc_cfg.g_h + 1) / 2);

        memcpy(image->planes[0], yuv, y_size);
        memcpy(image->planes[1], yuv + y_size, u_size);
        memcpy(image->planes[2], yuv + y_size + u_size, v_size);

        if (aom_codec_encode(&encoder, image,
                             frame_num, 1, 0) != AOM_CODEC_OK) {
            Logger::error("Encode failed at frame " +
                          std::to_string(frame_num));
            return false;
        }

        aom_codec_iter_t iter = nullptr;
        const aom_codec_cx_pkt_t* pkt;

        while ((pkt = aom_codec_get_cx_data(&encoder, &iter))) {
            if (pkt->kind == AOM_CODEC_CX_FRAME_PKT) {
                outfile.write(
                    static_cast<const char*>(pkt->data.frame.buf),
                    pkt->data.frame.sz);

                total_bytes += pkt->data.frame.sz;

                // Decode immediately
                if (aom_codec_decode(
                        &decoder,
                        static_cast<const uint8_t*>(
                            pkt->data.frame.buf),
                        pkt->data.frame.sz,
                        nullptr) != AOM_CODEC_OK) {
                    Logger::error("Decode failed");
                    return false;
                }

                aom_codec_iter_t diter = nullptr;
                aom_image_t* img;
                while ((img = aom_codec_get_frame(&decoder, &diter))) {
                    decoded_frames++;
                }
            }
        }

        total_encode_time_ms += t.elapsed_ms();
        encoded_frames++;
        return true;
    }

    bool flush(std::ofstream& outfile) {
        aom_codec_encode(&encoder, nullptr, 0, 0, 0);

        aom_codec_iter_t iter = nullptr;
        const aom_codec_cx_pkt_t* pkt;

        while ((pkt = aom_codec_get_cx_data(&encoder, &iter))) {
            if (pkt->kind == AOM_CODEC_CX_FRAME_PKT) {
                outfile.write(
                    static_cast<const char*>(pkt->data.frame.buf),
                    pkt->data.frame.sz);
                total_bytes += pkt->data.frame.sz;

                aom_codec_decode(
                    &decoder,
                    static_cast<const uint8_t*>(
                        pkt->data.frame.buf),
                    pkt->data.frame.sz,
                    nullptr);
            }
        }

        aom_codec_decode(&decoder, nullptr, 0, nullptr);

        aom_codec_iter_t diter = nullptr;
        aom_image_t* img;
        while ((img = aom_codec_get_frame(&decoder, &diter))) {
            decoded_frames++;
        }
        return true;
    }

    void print_stats() const {
        Logger::stats("Encoded frames: " +
                      std::to_string(encoded_frames));
        Logger::stats("Decoded frames: " +
                      std::to_string(decoded_frames));
        Logger::stats("Total output size: " +
                      std::to_string(total_bytes) + " bytes");
        Logger::stats("Average encode time/frame: " +
                      std::to_string(
                          total_encode_time_ms / encoded_frames) +
                      " ms");
        Logger::stats("Encoding FPS: " +
                      std::to_string(
                          encoded_frames /
                          (total_encode_time_ms / 1000.0)));
    }
};

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main(int argc, char* argv[]) {
    Config cfg{
        "input.yuv",
        "output.ivf",
        1920, 1080,
        30,
        300,
        2000,
        6,
        4
    };

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-i")) cfg.input_file = argv[++i];
        else if (!strcmp(argv[i], "-o")) cfg.output_file = argv[++i];
    }

    std::ifstream infile(cfg.input_file, std::ios::binary);
    std::ofstream outfile(cfg.output_file, std::ios::binary);

    if (!infile || !outfile) {
        Logger::error("Failed to open input/output file");
        return 1;
    }

    AOMCodec codec;
    if (!codec.init(cfg)) {
        return 1;
    }

    size_t frame_size = cfg.width * cfg.height * 3 / 2;
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[frame_size]);

    int frame = 0;
    Timer total;
    while (frame < cfg.frames &&
           infile.read(reinterpret_cast<char*>(buffer.get()),
                       frame_size)) {
        if (!codec.encode_and_decode(buffer.get(),
                                     outfile, frame)) {
            return 1;
        }
        frame++;
    }

    codec.flush(outfile);

    codec.print_stats();
    Logger::timing("Total wall time", total.elapsed_ms());
    Logger::info("Encoding + decoding completed successfully");
    return 0;
}
