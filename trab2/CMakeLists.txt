cmake_minimum_required(VERSION 3.14)
project(GolombCoding VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/includes)

# Add subdirectories
add_subdirectory(src/Part_I)
add_subdirectory(src/Part_II)
add_subdirectory(src/Part_III)
add_subdirectory(src/Part_IV)
add_subdirectory(src/Part_V)
