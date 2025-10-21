cmake_minimum_required(VERSION 3.16)
project(image_effects)

set(CMAKE_BUILD_TYPE "Release")
# set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -fsanitize=address")

set(BASE_DIR ${CMAKE_SOURCE_DIR})
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BASE_DIR}/bin)

add_executable(image_effects
    src/exe2/main.cpp
    src/exe2/Image.cpp
    src/exe2/Negative.cpp
    src/exe2/Mirror.cpp
    src/exe2/Rotate.cpp
    src/exe2/Intensity.cpp
)