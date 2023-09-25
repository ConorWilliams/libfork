cmake_minimum_required(VERSION 3.14)

project(bench_libfork LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(libfork REQUIRED)
endif()

find_package(benchmark REQUIRED)
find_package(Taskflow REQUIRED)
find_package(TBB REQUIRED)
find_package(OpenMP REQUIRED)


# ---- Benchmarks ----

file(GLOB_RECURSE BENCH_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/**/*.cpp")

add_executable(benchmark ${BENCH_SOURCES})

target_link_libraries(benchmark 
    PRIVATE 
        libfork::libfork 
        OpenMP::OpenMP_CXX 
        TBB::tbb 
        Taskflow::Taskflow 
        benchmark::benchmark_main
)



# ---- End-of-file commands ----
add_folders(benchmarks)
