# ============================================================================
# AOM Clang Configuration
# ============================================================================

# Compiler
set(CMAKE_C_COMPILER "clang" CACHE STRING "C compiler")
set(CMAKE_CXX_COMPILER "clang++" CACHE STRING "C++ compiler")

# Build type
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type")

# AOM options
set(CONFIG_AV1_ENCODER 1 CACHE STRING "Enable encoder")
set(CONFIG_AV1_DECODER 1 CACHE STRING "Enable decoder")
set(ENABLE_TESTS 0 CACHE BOOL "Skip tests")
set(ENABLE_DOCS 0 CACHE BOOL "Skip docs")
set(ENABLE_TOOLS 1 CACHE BOOL "Build tools")
set(ENABLE_EXAMPLES 1 CACHE BOOL "Build examples")

# Codec features
set(CONFIG_MULTITHREAD 1 CACHE STRING "Multi-threading")
set(CONFIG_RUNTIME_CPU_DETECT 1 CACHE STRING "CPU detection")

message(STATUS "========================================")
message(STATUS "AOM Clang Build Configuration")
message(STATUS "========================================")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Encoder: ${CONFIG_AV1_ENCODER}")
message(STATUS "Decoder: ${CONFIG_AV1_DECODER}")
message(STATUS "Tools: ${ENABLE_TOOLS}")
message(STATUS "========================================")