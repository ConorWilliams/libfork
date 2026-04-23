cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(baremetal_benchmarks LANGUAGES CXX)

add_library(baremetal_benchmarks)

target_sources(baremetal_benchmarks PRIVATE fib.cpp)

target_link_libraries(baremetal_benchmarks PUBLIC benchmark_common)
