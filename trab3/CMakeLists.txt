cmake_minimum_required(VERSION 3.15)
project(SafetensorsCompressor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -march=native")

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZSTD REQUIRED libzstd)
pkg_check_modules(LZMA REQUIRED liblzma)

include_directories(
    ${CMAKE_SOURCE_DIR}/includes
    ${ZSTD_INCLUDE_DIRS}
    ${LZMA_INCLUDE_DIRS}
)

set(SOURCES
    src/main.cpp
    src/safetensors_parser.cpp
    src/preprocessor.cpp
    src/compressor.cpp
    src/benchmarker.cpp
)

add_executable(compressor ${SOURCES})

target_link_libraries(compressor
    ${ZSTD_LIBRARIES}
    ${LZMA_LIBRARIES}
    pthread
)

set_target_properties(compressor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
