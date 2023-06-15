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

add_executable(overhead.out overhead.cpp)
target_link_libraries(overhead.out PRIVATE libfork::libfork nanobench)

# ---- End-of-file commands ----

add_folders(benchmarks)
