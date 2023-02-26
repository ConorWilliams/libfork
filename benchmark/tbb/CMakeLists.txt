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

find_package(TBB REQUIRED)

# ---- Benchmarks ----
add_executable(tbb_fib.bench fib.cpp)
target_link_libraries(tbb_fib.bench PRIVATE libfork::libfork nanobench TBB::tbb)

add_executable(tbb_reduce.bench reduce.cpp)
target_link_libraries(tbb_reduce.bench PRIVATE libfork::libfork nanobench TBB::tbb)

add_executable(tbb_matmul.bench matmul.cpp)
target_link_libraries(tbb_matmul.bench PRIVATE libfork::libfork nanobench TBB::tbb)

add_executable(tbb_dfs.bench dfs.cpp)
target_link_libraries(tbb_dfs.bench PRIVATE libfork::libfork nanobench TBB::tbb)

# ---- End-of-file commands ----
add_folders(benchmarks)
