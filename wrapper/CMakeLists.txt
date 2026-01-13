cmake_minimum_required(VERSION 3.10)
project(AOMWrapper VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# AOM Library Configuration
# ============================================================================
set(AOM_INCLUDE_DIR "D:/AVXsession/aom" CACHE PATH "AOM include directory")
set(AOM_BUILD_DIR "D:/AVXsession/aom/build" CACHE PATH "AOM build directory")

find_library(AOM_LIBRARY
    NAMES aom libaom
    PATHS ${AOM_BUILD_DIR}
    NO_DEFAULT_PATH
)

if(NOT AOM_LIBRARY)
    message(FATAL_ERROR "AOM library not found at ${AOM_BUILD_DIR}")
endif()

message(STATUS "Found AOM library: ${AOM_LIBRARY}")

# ============================================================================
# Build Shared Library (DLL)
# ============================================================================
add_library(aom_wrapper SHARED
    aom_wrapper.cpp
    aom_wrapper.h
)

target_include_directories(aom_wrapper
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${AOM_INCLUDE_DIR}
)

target_link_libraries(aom_wrapper PRIVATE ${AOM_LIBRARY})

# Windows-specific
if(WIN32)
    target_link_libraries(aom_wrapper PRIVATE ws2_32 winmm)
endif()

# Set DLL export macro
target_compile_definitions(aom_wrapper PRIVATE AOM_WRAPPER_EXPORTS)


# ============================================================================
# Installation
# ============================================================================
install(TARGETS aom_wrapper
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES aom_wrapper.h DESTINATION include)

# ============================================================================
# Configuration Summary
# ============================================================================
message(STATUS "===========================================")
message(STATUS "AOM Wrapper Library Configuration")
message(STATUS "===========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "AOM library: ${AOM_LIBRARY}")
message(STATUS "AOM include: ${AOM_INCLUDE_DIR}")
message(STATUS "Output: ${CMAKE_BINARY_DIR}")
message(STATUS "===========================================")