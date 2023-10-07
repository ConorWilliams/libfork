cmake_minimum_required(VERSION 3.14)

project(bench_libfork LANGUAGES CXX C)

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

file(GLOB_RECURSE BENCH_C_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/**/*.c")

file(GLOB_RECURSE BENCH_CXX_FIB CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/fib/*.cpp")
file(GLOB_RECURSE BENCH_CXX_REDUCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/reduce/*.cpp")
file(GLOB_RECURSE BENCH_CXX_UTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/uts/*.cpp")
file(GLOB_RECURSE BENCH_CXX_NQUEENS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/nqueens/*.cpp")
file(GLOB_RECURSE BENCH_CXX_MATMUL CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/matmul/*.cpp")
file(GLOB_RECURSE BENCH_CXX_INTEGRATE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/integrate/*.cpp")

add_executable(benchmark 
  ${BENCH_C_SOURCES} 
  ${BENCH_CXX_FIB} 
  ${BENCH_CXX_REDUCE} 
  ${BENCH_CXX_UTS} 
  ${BENCH_CXX_NQUEENS}
  ${BENCH_CXX_MATMUL}
  ${BENCH_CXX_INTEGRATE}
)

#  UTS needs a recursion depth of 18'000 frames!
target_compile_definitions(benchmark PRIVATE LF_ASYNC_STACK_SIZE=2048)

target_link_libraries(benchmark 
    PRIVATE 
        libfork::libfork 
        OpenMP::OpenMP_CXX 
        TBB::tbb 
        Taskflow::Taskflow 
        benchmark::benchmark_main
)

# ---- Nowa ---- #

option(NOWA_ROOT "Location of nowa build dir" OFF)

if(NOT NOWA_ROOT)
    message(FATAL_ERROR "Please specify the location of the nowa build dir with -DNOWA_ROOT=<path>")
endif()

target_include_directories(benchmark SYSTEM PRIVATE ${NOWA_ROOT}/include)
target_link_libraries(benchmark PRIVATE ${NOWA_ROOT}/lib/libfibril.so)

# ---- Coro benchmarks ----

add_executable(coro_bench ${CMAKE_CURRENT_SOURCE_DIR}/source/coroutine/incremental.cpp)

target_link_libraries(coro_bench 
    PRIVATE 
        libfork::libfork 
        benchmark::benchmark_main
)

# ---- End-of-file commands ----
add_folders(benchmarks)
