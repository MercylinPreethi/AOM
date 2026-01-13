// test.cpp - AOM Wrapper Test with Custom Configuration (C++98 compatible)
#include "aom_wrapper.h"
#include <iostream>
#include <sstream>

// Helper function to convert int to string
std::string int_to_string(int value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

int main() {
    std::cout << "========================================\n";
    std::cout << "AOM Wrapper Test - Custom Configuration\n";
    std::cout << "========================================\n";
    std::cout << "Library version: " << aom_wrapper_version() << "\n\n";
    
    
    
    std::cout << "Testing ENCODE with custom config...\n";
    std::cout << "----------------------------------------\n";
    
    // Create configuration struct
    AomEncodeParams encode_params;
    aom_encode_params_default(&encode_params);
    
    
    encode_params.width = 0;      
    encode_params.height = 0;      

    encode_params.fps = 120;        

    encode_params.frames = 300;   
    
    encode_params.bitrate = 5000;  
    
    encode_params.cpu_used = 6;  

    encode_params.threads = 2;      

    encode_params.verbose = 1;      
    
   
    std::cout << "Encode Configuration:\n";
    std::cout << "  Resolution: ";
    if (encode_params.width == 0) {
        std::cout << "auto-detect";
    } else {
        std::cout << encode_params.width;
    }
    std::cout << " x ";
    if (encode_params.height == 0) {
        std::cout << "auto-detect";
    } else {
        std::cout << encode_params.height;
    }
    std::cout << "\n";
    
    std::cout << "  FPS: " << encode_params.fps << "\n";
    
    std::cout << "  Frames: ";
    if (encode_params.frames == 0) {
        std::cout << "all";
    } else {
        std::cout << encode_params.frames;
    }
    std::cout << "\n";
    
    std::cout << "  Bitrate: " << encode_params.bitrate << " kbps\n";
    std::cout << "  CPU preset: " << encode_params.cpu_used << " (0=best quality, 8=fastest)\n";
    
    std::cout << "  Threads: ";
    if (encode_params.threads == 0) {
        std::cout << "auto";
    } else {
        std::cout << encode_params.threads;
    }
    std::cout << "\n";
    
    std::cout << "  Verbose: " << encode_params.verbose << "\n";
    std::cout << "\n";
    
    
    int result = aom_encode(
        "C:/Users/mcw/Downloads/Bosphorus_1920x1080_120fps_420_8bit_YUV_RAW/Bosphorus_1920x1080_120fps_420_8bit_YUV.yuv",
        "test_output.ivf",
        &encode_params  
    );
    
    if (result == AOM_WRAPPER_OK) {
        std::cout << "\n[SUCCESS] Encoding successful!\n\n";
    } else {
        std::cerr << "\n[ERROR] Encoding failed: " << aom_wrapper_error_string(result) << "\n\n";
        return 1;
    }
    
    
    std::cout << "Testing DECODE with custom config...\n";
    std::cout << "----------------------------------------\n";
    
    // Create decode configuration struct
    AomDecodeParams decode_params;
    aom_decode_params_default(&decode_params); 
    
    
    decode_params.frames = 0;       // Decode all frames
    
   
    decode_params.verbose = 1;      // Normal logging
 
    
    std::cout << "Decode Configuration:\n";
    std::cout << "  Frames: ";
    if (decode_params.frames == 0) {
        std::cout << "all";
    } else {
        std::cout << decode_params.frames;
    }
    std::cout << "\n";
    std::cout << "  Verbose: " << decode_params.verbose << "\n";
    std::cout << "\n";

    
    result = aom_decode(
        "test_output.ivf",
        "test_decoded.y4m",
        &decode_params  
    );
    
    if (result == AOM_WRAPPER_OK) {
        std::cout << "\n[SUCCESS] Decoding successful!\n\n";
    } else {
        std::cerr << "\n[ERROR] Decoding failed: " << aom_wrapper_error_string(result) << "\n\n";
        return 1;
    }
    
    
    
    std::cout << "========================================\n";
    std::cout << "[SUCCESS] All tests passed!\n";
    std::cout << "========================================\n";
    std::cout << "\nGenerated files:\n";
    std::cout << "  - test_output.ivf (compressed video)\n";
    std::cout << "  - test_decoded.y4m (decoded video)\n";
    std::cout << "\nTo play decoded video:\n";
    std::cout << "  ffplay test_decoded.y4m\n";
    std::cout << "\n";
    
    return 0;
}