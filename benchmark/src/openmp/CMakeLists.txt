cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(openmp_benchmarks LANGUAGES CXX)

add_library(openmp_benchmarks)

target_sources(openmp_benchmarks
  PRIVATE
    fib_omp.cpp uts_omp.cpp
)

target_link_libraries(openmp_benchmarks
  PUBLIC
    benchmark_common
    libfork::libfork
    OpenMP::OpenMP_CXX
)

target_include_directories(openmp_benchmarks
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libfork
)
