cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(libfork_benchmark LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  message(WARNING "It is recommended to build benchmarks in Release mode for accurate results.")
endif()

# ---- Dependencies ----

find_package(benchmark REQUIRED)

# ---- Benchmarks ----

add_subdirectory(lib)

add_subdirectory(src/serial)
add_subdirectory(src/baremetal)
add_subdirectory(src/libfork)

# WHOLE_ARCHIVE ensures benchmark registrations (global initialisers) are not
# dropped by the linker when pulling objects from the static libraries above.
add_executable(libfork_benchmark src/benchmarks.cpp)

target_link_libraries(libfork_benchmark
  PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,serial_benchmarks,baremetal_benchmarks,libfork_benchmarks>
    benchmark::benchmark_main
)

if(BUILD_TESTING)
  add_test(NAME Benchmark
    COMMAND libfork_benchmark --benchmark_dry_run --benchmark_filter=^test/
  )
endif()

# ---- OpenMP Benchmarks ----

find_package(OpenMP REQUIRED)

if(OpenMP_CXX_FOUND)

  add_subdirectory(src/openmp)

  target_link_libraries(libfork_benchmark
  PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,openmp_benchmarks>
    OpenMP::OpenMP_CXX
  )
endif()


