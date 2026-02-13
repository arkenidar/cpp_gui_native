cmake_minimum_required(VERSION 3.22.1)
project(guicpp_native LANGUAGES CXX)

add_library(guicpp_native SHARED
    bootstrap.cpp
)

target_compile_features(guicpp_native PRIVATE cxx_std_20)

find_library(log-lib log)

target_link_libraries(guicpp_native PRIVATE
    android
    ${log-lib}
)
