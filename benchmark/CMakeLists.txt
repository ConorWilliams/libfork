cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(libfork_benchmark LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  message(WARNING "It is recommended to build benchmarks in Release mode for accurate results.")
endif()

# ---- Required dependencies ----

find_package(benchmark REQUIRED)

# ---- Benchmarks ----

file(GLOB_RECURSE BENCH_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(libfork_benchmark ${BENCH_SOURCES})

target_link_libraries(libfork_benchmark PRIVATE libfork::libfork benchmark::benchmark_main)


