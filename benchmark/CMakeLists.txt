cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(libfork_benchmark LANGUAGES C CXX)

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
      src/libfork_benchmark/uts/uts.hpp
    BASE_DIRS
      src
)

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
    src/libfork_benchmark/fib/serial_return.cpp
)

# ---- Baremetal ----

target_sources(libfork_benchmark
  PRIVATE
    src/libfork_benchmark/fib/baremetal.cpp
)

# ---- Libfork ----

target_sources(libfork_benchmark
  PRIVATE
    src/libfork_benchmark/fib/libfork.cpp
)

target_link_libraries(libfork_benchmark
  PRIVATE
    libfork::libfork
)

# ---- UTS C library ----

add_library(uts_c OBJECT
  src/libfork_benchmark/uts/external/uts.c
  src/libfork_benchmark/uts/external/rng/brg_sha1.c
)

target_link_libraries(libfork_benchmark PRIVATE uts_c)

# ---- UTS ----

target_sources(libfork_benchmark
  PRIVATE
    src/libfork_benchmark/uts/serial.cpp
    src/libfork_benchmark/uts/libfork.cpp
)
