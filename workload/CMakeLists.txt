cmake_minimum_required(VERSION 3.10)
project(AOMWorkload)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Find AOM Library
# ============================================================================

# Option 1: If AOM is installed system-wide
# find_package(aom REQUIRED)

# Option 2: If you built AOM locally, specify paths manually
# Set these paths to your AOM build directory
set(AOM_INCLUDE_DIR "D:/AVXsession/aom")
set(AOM_BUILD_DIR "D:/AVXsession/aom/build")

# Find AOM library
find_library(AOM_LIBRARY
    NAMES aom libaom
    PATHS ${AOM_BUILD_DIR}
    NO_DEFAULT_PATH
)

if(NOT AOM_LIBRARY)
    message(FATAL_ERROR "AOM library not found! Please build AOM first or set AOM_BUILD_DIR correctly.")
endif()

message(STATUS "Found AOM library: ${AOM_LIBRARY}")
message(STATUS "AOM include directory: ${AOM_INCLUDE_DIR}")

# ============================================================================
# Build Configuration
# ============================================================================

# Add executable
add_executable(workload workload.cpp)
target_compile_features(workload PRIVATE cxx_std_17)

# Include directories
target_include_directories(workload PRIVATE
    ${AOM_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(workload PRIVATE
    ${AOM_LIBRARY}
)

# Windows-specific settings
if(WIN32)
    # Link Windows libraries that AOM depends on
    target_link_libraries(workload PRIVATE
        ws2_32      # Windows Sockets
        winmm       # Windows Multimedia
    )
    
    # Enable static runtime if needed
    # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Linux-specific settings
if(UNIX AND NOT APPLE)
    # Link pthread for multi-threading
    find_package(Threads REQUIRED)
    target_link_libraries(workload PRIVATE
        Threads::Threads
        m  # Math library
    )
endif()

# macOS-specific settings
if(APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(workload PRIVATE
        Threads::Threads
    )
endif()

# ============================================================================
# Build Options
# ============================================================================

# Optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(workload PRIVATE /O2 /W4)
    else()
        target_compile_options(workload PRIVATE -O3 -march=native)
    endif()
endif()

# Debug flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        target_compile_options(workload PRIVATE /Od /Zi)
    else()
        target_compile_options(workload PRIVATE -g -O0)
    endif()
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS workload
    RUNTIME DESTINATION bin
)

# ============================================================================
# Print Configuration Summary
# ============================================================================

message(STATUS "===========================================")
message(STATUS "AOM Workload Build Configuration")
message(STATUS "===========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "AOM library: ${AOM_LIBRARY}")
message(STATUS "AOM include: ${AOM_INCLUDE_DIR}")
message(STATUS "===========================================")