cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(libfork_benchmark LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  message(WARNING "It is recommended to build benchmarks in Release mode for accurate results.")
endif()

# ---- Benchmarks ----

find_package(benchmark REQUIRED)

add_executable(libfork_benchmark)

target_link_libraries(libfork_benchmark
  PRIVATE
    benchmark::benchmark_main
)

# Headers (shared between all benchmarks)
target_sources(libfork_benchmark
  PRIVATE
    FILE_SET HEADERS FILES
      benchmark/src/libfork_benchmark/common.hpp
    BASE_DIRS
      benchmark/src
)

# ---- Libfork ----

target_sources(libfork_benchmark
  PRIVATE
    benchmark/src/libfork_benchmark/fib/lf.cpp
)

target_link_libraries(libfork_benchmark
  PRIVATE
    libfork::libfork
)


