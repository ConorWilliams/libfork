cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(benchmark_common LANGUAGES CXX)

add_library(benchmark_common STATIC)

target_sources(benchmark_common
  PRIVATE
    uts.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      common.hpp
      fib.hpp
      uts.hpp
)

target_link_libraries(benchmark_common
  PUBLIC
    benchmark::benchmark
    uts_c
    libfork::libfork
)
