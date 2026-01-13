// aom_wrapper.h - Simple AOM AV1 Encoder/Decoder Interface
// Public API for encoding and decoding with minimal arguments

#ifndef AOM_WRAPPER_H
#define AOM_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Platform-specific DLL export/import
// ============================================================================
#ifdef _WIN32
    #ifdef AOM_WRAPPER_EXPORTS
        #define AOM_WRAPPER_API __declspec(dllexport)
    #else
        #define AOM_WRAPPER_API __declspec(dllimport)
    #endif
#else
    #define AOM_WRAPPER_API __attribute__((visibility("default")))
#endif

// ============================================================================
// Return Codes
// ============================================================================
typedef enum {
    AOM_WRAPPER_OK = 0,
    AOM_WRAPPER_ERROR_INPUT_FILE = -1,
    AOM_WRAPPER_ERROR_OUTPUT_FILE = -2,
    AOM_WRAPPER_ERROR_ENCODER_INIT = -3,
    AOM_WRAPPER_ERROR_DECODER_INIT = -4,
    AOM_WRAPPER_ERROR_ENCODE_FAILED = -5,
    AOM_WRAPPER_ERROR_DECODE_FAILED = -6,
    AOM_WRAPPER_ERROR_INVALID_PARAMS = -7,
    AOM_WRAPPER_ERROR_OUT_OF_MEMORY = -8
} AomWrapperResult;

// ============================================================================
// Simple Encoding Interface
// ============================================================================

/**
 * Encode YUV file to AV1 IVF with default settings
 * 
 * @param input_yuv_path  Path to input YUV file
 * @param output_ivf_path Path to output IVF file
 * @return AOM_WRAPPER_OK on success, error code otherwise
 * 
 * Default settings:
 * - Resolution: Auto-detected from filename or 1920x1080
 * - FPS: 30
 * - Bitrate: 2000 kbps
 * - CPU preset: 6 (balanced)
 * - Threads: Auto (CPU cores)
 */
AOM_WRAPPER_API int aom_encode_simple(
    const char* input_yuv_path,
    const char* output_ivf_path
);

/**
 * Decode IVF file to Y4M with default settings
 * 
 * @param input_ivf_path  Path to input IVF file
 * @param output_y4m_path Path to output Y4M file
 * @return AOM_WRAPPER_OK on success, error code otherwise
 */
AOM_WRAPPER_API int aom_decode_simple(
    const char* input_ivf_path,
    const char* output_y4m_path
);

/**
 * Encode then decode (round-trip test)
 * 
 * @param input_yuv_path  Path to input YUV file
 * @param output_y4m_path Path to output Y4M file
 * @return AOM_WRAPPER_OK on success, error code otherwise
 */
AOM_WRAPPER_API int aom_roundtrip_simple(
    const char* input_yuv_path,
    const char* output_y4m_path
);

// ============================================================================
// Advanced Encoding Interface (Optional Parameters)
// ============================================================================

typedef struct {
    int width;           // Video width (0 = auto-detect)
    int height;          // Video height (0 = auto-detect)
    int fps;             // Frame rate (0 = default 30)
    int frames;          // Number of frames to encode (0 = all)
    int bitrate;         // Target bitrate in kbps (0 = default 2000)
    int cpu_used;        // CPU preset 0-8 (0 = slowest/best, 8 = fastest/worst)
    int threads;         // Number of threads (0 = auto)
    int verbose;         // Verbose logging (0 = quiet, 1 = normal, 2 = verbose)
} AomEncodeParams;

/**
 * Get default encoding parameters
 * 
 * @param params Pointer to params structure to fill
 */
AOM_WRAPPER_API void aom_encode_params_default(AomEncodeParams* params);

/**
 * Encode with custom parameters
 * 
 * @param input_yuv_path  Path to input YUV file
 * @param output_ivf_path Path to output IVF file
 * @param params          Encoding parameters (NULL for defaults)
 * @return AOM_WRAPPER_OK on success, error code otherwise
 */
AOM_WRAPPER_API int aom_encode(
    const char* input_yuv_path,
    const char* output_ivf_path,
    const AomEncodeParams* params
);

// ============================================================================
// Advanced Decoding Interface (Optional Parameters)
// ============================================================================

typedef struct {
    int frames;          // Number of frames to decode (0 = all)
    int verbose;         // Verbose logging (0 = quiet, 1 = normal, 2 = verbose)
} AomDecodeParams;

/**
 * Get default decoding parameters
 * 
 * @param params Pointer to params structure to fill
 */
AOM_WRAPPER_API void aom_decode_params_default(AomDecodeParams* params);

/**
 * Decode with custom parameters
 * 
 * @param input_ivf_path  Path to input IVF file
 * @param output_y4m_path Path to output Y4M file
 * @param params          Decoding parameters (NULL for defaults)
 * @return AOM_WRAPPER_OK on success, error code otherwise
 */
AOM_WRAPPER_API int aom_decode(
    const char* input_ivf_path,
    const char* output_y4m_path,
    const AomDecodeParams* params
);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Get error message for return code
 * 
 * @param result Return code from encode/decode functions
 * @return Human-readable error message
 */
AOM_WRAPPER_API const char* aom_wrapper_error_string(int result);

/**
 * Get library version string
 * 
 * @return Version string (e.g., "1.0.0")
 */
AOM_WRAPPER_API const char* aom_wrapper_version(void);

/**
 * Auto-detect video resolution from filename
 * Examples: "video_1920x1080.yuv" -> width=1920, height=1080
 * 
 * @param filename Input filename
 * @param width    Pointer to store detected width
 * @param height   Pointer to store detected height
 * @return 1 if detected, 0 if not found
 */
AOM_WRAPPER_API int aom_detect_resolution(
    const char* filename,
    int* width,
    int* height
);

#ifdef __cplusplus
}
#endif

#endif // AOM_WRAPPER_H