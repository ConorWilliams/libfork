cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(openmp_benchmarks LANGUAGES CXX)

add_library(openmp_benchmarks)

# OpenMP compiles with -fopenmp which conflicts with the shared std.pcm (built
# without OpenMP). Disable module scanning so CMake doesn't inject the
# incompatible modmap for this target.
set_target_properties(openmp_benchmarks PROPERTIES CXX_SCAN_FOR_MODULES OFF)

# Signal to shared benchmark headers that this target cannot `import std;`
# and must use textual standard headers instead.
target_compile_definitions(openmp_benchmarks PRIVATE LF_BENCH_NO_IMPORT_STD)

target_sources(openmp_benchmarks
  PRIVATE
    fib.cpp uts.cpp
)

target_link_libraries(openmp_benchmarks
  PUBLIC
    benchmark_common
    OpenMP::OpenMP_CXX
)
