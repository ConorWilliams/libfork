cmake_minimum_required(VERSION 3.14) 

project(bench_libfork LANGUAGES CXX)

include(../../cmake/project-is-top-level.cmake)
include(../../cmake/folders.cmake)
include(../../cmake/CPM.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(libfork REQUIRED)
endif()

CPMAddPackage("gh:martinus/nanobench#v4.1.0")

# ---- Benchmarks ----

add_executable(lf_fib.bench fib.cpp)
target_link_libraries(lf_fib.bench PRIVATE libfork::libfork nanobench)

add_executable(lf_reduce.bench reduce.cpp)
target_link_libraries(lf_reduce.bench PRIVATE libfork::libfork nanobench)

add_executable(lf_matmul.bench matmul.cpp)
target_link_libraries(lf_matmul.bench PRIVATE libfork::libfork nanobench)

add_executable(lf_dfs.bench dfs.cpp)
target_link_libraries(lf_dfs.bench PRIVATE libfork::libfork nanobench)

# ---- End-of-file commands ----

add_folders(benchmarks)


