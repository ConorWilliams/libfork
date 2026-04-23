cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(libfork_benchmark LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  message(WARNING "It is recommended to build benchmarks in Release mode for accurate results.")
endif()

# ---- Dependencies ----

find_package(benchmark REQUIRED)
find_package(OpenMP REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/uts external/uts)

# ---- Benchmarks ----

add_subdirectory(lib)

add_subdirectory(src/serial)
add_subdirectory(src/baremetal)
add_subdirectory(src/libfork)
# add_subdirectory(src/openmp)

# WHOLE_ARCHIVE ensures benchmark registrations (global initialisers) are not
# dropped by the linker when pulling objects from the static libraries above.
add_executable(libfork_benchmark src/benchmarks.cpp)

target_link_libraries(libfork_benchmark
  PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,serial_benchmarks,baremetal_benchmarks,libfork_benchmarks>
    benchmark::benchmark_main
    # OpenMP::OpenMP_CXX
)

if(BUILD_TESTING)
  add_test(NAME Benchmark
    COMMAND libfork_benchmark --benchmark_dry_run --benchmark_filter=^test/
  )
endif()
