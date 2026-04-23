cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(libfork_benchmark LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  message(WARNING "It is recommended to build benchmarks in Release mode for accurate results.")
endif()

# ---- Dependencies ----

find_package(benchmark REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/uts external/uts)

# ---- Benchmarks ----

add_subdirectory(src/lib)
add_subdirectory(src/serial)
add_subdirectory(src/baremetal)
add_subdirectory(src/libfork)
