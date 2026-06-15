cmake_minimum_required(VERSION 3.20)
project(fslog LANGUAGES CXX)

add_library(fslog INTERFACE)
add_library(fslog::fslog ALIAS fslog)

target_include_directories(fslog INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_features(fslog INTERFACE cxx_std_17)
