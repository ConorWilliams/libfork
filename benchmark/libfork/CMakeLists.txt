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

add_executable(fib fib.cpp)
target_link_libraries(fib PRIVATE libfork::libfork nanobench)

add_executable(reduce reduce.cpp)
target_link_libraries(reduce PRIVATE libfork::libfork nanobench)

add_executable(matmul matmul.cpp)
target_link_libraries(matmul PRIVATE libfork::libfork nanobench)

add_executable(dfs dfs.cpp)
target_link_libraries(dfs PRIVATE libfork::libfork nanobench)

# ---- End-of-file commands ----

add_folders(bench_libfork)


