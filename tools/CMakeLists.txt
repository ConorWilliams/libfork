cmake_minimum_required(VERSION 3.14)

project(libforkTools LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)
include(../cmake/windows-set-path.cmake)

# Tools

add_executable(make_single_header make_single_header.cpp)
target_compile_features(make_single_header PRIVATE cxx_std_20)

add_folders(Tools)
