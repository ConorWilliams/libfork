cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(libfork_benchmarks LANGUAGES CXX)

add_library(libfork_benchmarks)

target_sources(libfork_benchmarks PRIVATE fib.cpp uts.cpp)

target_link_libraries(libfork_benchmarks PUBLIC benchmark_common)
