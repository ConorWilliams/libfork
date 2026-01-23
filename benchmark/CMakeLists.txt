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
      src/libfork_benchmark/common.hpp
      src/libfork_benchmark/fib/fib.hpp
    BASE_DIRS
      src
)

# TODO: --size test

if(BUILD_TESTING)
  add_test(NAME Benchmark
    COMMAND
      libfork_benchmark --benchmark_dry_run --benchmark_filter=^test/
  )
endif()

# ---- Serial ----

target_sources(libfork_benchmark
  PRIVATE
    src/libfork_benchmark/fib/serial.cpp
)

# ---- Libfork ----
#
# target_sources(libfork_benchmark
#   PRIVATE
#     src/libfork_benchmark/fib/fib.cpp
# )
#
# target_link_libraries(libfork_benchmark
#   PRIVATE
#     libfork::libfork
# )
