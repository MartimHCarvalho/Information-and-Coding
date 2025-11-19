cmake_minimum_required(VERSION 3.10)
project(SafeTensorsCompressor)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")

# Find required libraries using pkg-config
find_package(PkgConfig REQUIRED)

# Find all compression libraries
pkg_check_modules(ZSTD REQUIRED libzstd)
pkg_check_modules(LZ4 REQUIRED liblz4)
pkg_check_modules(ZLIB REQUIRED zlib)
pkg_check_modules(LIBLZMA REQUIRED liblzma)

# Source files
set(SOURCES
    src/main.cpp
    src/compressor.cpp
    src/preprocessor.cpp
    src/safetensors_parser.cpp
    src/benchmarker.cpp
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ZSTD_INCLUDE_DIRS}
    ${LZ4_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${LIBLZMA_INCLUDE_DIRS}
)

# Executable
add_executable(compressor ${SOURCES})

# Link libraries
target_link_libraries(compressor
    ${ZSTD_LIBRARIES}
    ${LZ4_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${LIBLZMA_LIBRARIES}
)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(compressor PRIVATE
        -Wall
        -Wextra
        -O3
        -march=native
    )
endif()

# Set output directory
set_target_properties(compressor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)

# Print library information
message(STATUS "ZSTD libraries: ${ZSTD_LIBRARIES}")
message(STATUS "ZSTD include dirs: ${ZSTD_INCLUDE_DIRS}")
message(STATUS "LZ4 libraries: ${LZ4_LIBRARIES}")
message(STATUS "LZ4 include dirs: ${LZ4_INCLUDE_DIRS}")
message(STATUS "ZLIB libraries: ${ZLIB_LIBRARIES}")
message(STATUS "LibLZMA libraries: ${LIBLZMA_LIBRARIES}")