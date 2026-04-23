cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(openmp_benchmarks LANGUAGES CXX)

add_library(openmp_benchmarks)

target_sources(openmp_benchmarks
  PRIVATE
    fib.cpp uts.cpp
)

target_link_libraries(openmp_benchmarks
  PUBLIC
    benchmark_common
    OpenMP::OpenMP_CXX
)
