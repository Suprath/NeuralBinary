cmake_minimum_required(VERSION 3.16)
project(NeuralBinaryZlib VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Optimization Flags
if(MSVC)
    add_compile_options(/O2 /arch:AVX2)
else()
    add_compile_options(-O3 -march=native)
    if(APPLE)
        add_compile_options(-march=armv8-a+crc)
    endif()
endif()

include_directories(modernized)

set(SOURCES
    modernized/modernized_official_adler32.cpp
    modernized/modernized_zlib_crc32.cpp
    modernized/modernized_zlib_c_api.cpp
)

# Static Library
add_library(neuralbinary_zlib_static STATIC ${SOURCES})
set_target_properties(neuralbinary_zlib_static PROPERTIES OUTPUT_NAME "neuralbinary_zlib")

# Dynamic Shared Library (Drop-In replacement libz)
add_library(neuralbinary_zlib SHARED ${SOURCES})
set_target_properties(neuralbinary_zlib PROPERTIES OUTPUT_NAME "z_neuralbinary")

message(STATUS "NeuralBinary Modernized zlib C++20 configured successfully.")
