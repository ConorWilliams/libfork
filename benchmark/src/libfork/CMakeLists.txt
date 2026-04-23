cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(libfork_benchmarks LANGUAGES CXX)

add_library(libfork_benchmarks)

target_sources(libfork_benchmarks
  PRIVATE
    fib.cpp uts.cpp
  PRIVATE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      helpers.hpp
)

target_link_libraries(libfork_benchmarks PUBLIC benchmark_common libfork::libfork)
