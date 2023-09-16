cmake_minimum_required(VERSION 3.14)

project(bench_libfork LANGUAGES CXX)

include(../../cmake/project-is-top-level.cmake)
include(../../cmake/folders.cmake)

# ---- Dependencies ----
if(PROJECT_IS_TOP_LEVEL)
  find_package(libfork REQUIRED)
endif()

find_package(nanobench REQUIRED)

# ---- Benchmarks ----
add_executable(serial_fib.bench fib.cpp)
target_link_libraries(serial_fib.bench PRIVATE libfork::libfork nanobench)

add_executable(serial_reduce.bench reduce.cpp)
target_link_libraries(serial_reduce.bench PRIVATE libfork::libfork nanobench)

add_executable(serial_matmul.bench matmul.cpp)
target_link_libraries(serial_matmul.bench PRIVATE libfork::libfork nanobench)

add_executable(serial_dfs.bench dfs.cpp)
target_link_libraries(serial_dfs.bench PRIVATE libfork::libfork nanobench)

# ---- End-of-file commands ----
add_folders(benchmarks)
