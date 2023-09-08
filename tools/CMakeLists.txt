cmake_minimum_required(VERSION 3.14)

project(libforkTools LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)
include(../cmake/windows-set-path.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(libfork REQUIRED)
endif()

# Tools

add_executable(make_single_header make_single_header.cpp)
target_link_libraries(make_single_header PRIVATE libfork::libfork)
target_compile_features(make_single_header PRIVATE cxx_std_23)

add_folders(Tools)
