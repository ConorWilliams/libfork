cmake_minimum_required(VERSION 3.25) 

project(linforkBenchmark LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)
include(../cmake/CPM.cmake)

# ---- Set cmake to release ----

if(NOT CMAKE_BUILD_TYPE STREQUAL Release)
  message(WARNING "CMAKE_BUILD_TYPE is set to '${CMAKE_BUILD_TYPE}'; 'Release' is recommended for benchmarking.")
endif()

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(libfork REQUIRED)
endif()

find_package(OpenMP REQUIRED)
find_package(TBB REQUIRED)

CPMAddPackage("gh:martinus/nanobench#v4.1.0")

# ---- Tests ----

add_executable(libforkBench 
  source/main.cpp source/fib/fib.cpp 
  # source/matmul/matmul.cpp 
  # source/matmul/openmp.cpp
)

target_link_libraries(libforkBench PRIVATE 
  libfork::libfork 
  nanobench 
  OpenMP::OpenMP_CXX 
  TBB::tbb
)



add_custom_target(benchmark
  COMMAND libforkBench
  DEPENDS libforkBench
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


# ---- End-of-file commands ----

add_folders(Benchamark)


