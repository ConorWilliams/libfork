cmake_minimum_required(VERSION 3.14)

# ---- Warn if cmake is not release ----
if(NOT CMAKE_BUILD_TYPE STREQUAL Release)
  message(WARNING "CMAKE_BUILD_TYPE is set to '${CMAKE_BUILD_TYPE}' but, 'Release' is recommended for benchmarking.")
endif()

add_subdirectory(benchmark/serial)

add_subdirectory(benchmark/libfork)

find_package(OpenMP QUIET)

if(OpenMP_CXX_FOUND)
  add_subdirectory(benchmark/omp)
else()
  message(WARNING "OpenMP not found; OpenMP benchmarks will not be built.")
endif()

find_package(TBB QUIET)

if(TBB_FOUND)
  add_subdirectory(benchmark/tbb)
else()
  message(WARNING "Intel TBB not found; TBB benchmarks will not be built.")
endif()

# ---- End-of-file commands ----
add_folders(Benchamark)
