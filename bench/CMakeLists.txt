cmake_minimum_required(VERSION 3.14)

project(bench_libfork LANGUAGES CXX C)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Required dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(libfork REQUIRED)
endif()

find_package(benchmark REQUIRED)

# ---- Benchmarks ----

file(GLOB_RECURSE BENCH_C_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/**/*.c")
file(GLOB_RECURSE BENCH_SERIAL CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/**/serial.cpp")
file(GLOB_RECURSE BENCH_LIBFORK CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/source/**/libfork.cpp"
)

add_executable(
  benchmark ${BENCH_C_SOURCES} ${BENCH_SERIAL} ${BENCH_LIBFORK}
            ${CMAKE_CURRENT_SOURCE_DIR}/source/calibrate.cpp
)

target_link_libraries(benchmark PRIVATE libfork::libfork benchmark::benchmark_main)

option(LF_NO_CHECK "Disable benchmark verifications to speed up the benchmarks " OFF)

if(LF_NO_CHECK)
  target_compile_definitions(benchmark PRIVATE LF_NO_CHECK)
endif()

# --- Optional dependencies ---

find_package(TBB QUIET)

if(TBB_FOUND)
  file(GLOB_RECURSE BENCH_TBB CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/**/tbb.cpp")
  target_sources(benchmark PRIVATE ${BENCH_TBB})
  target_link_libraries(benchmark PRIVATE TBB::tbb)
else()
  message(WARNING "TBB not found, skipping TBB benchmarks")
endif()

find_package(OpenMP QUIET)

if(OpenMP_FOUND)
  file(GLOB_RECURSE BENCH_OpenMP CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/**/omp.cpp")
  target_sources(benchmark PRIVATE ${BENCH_OpenMP})
  target_link_libraries(benchmark PRIVATE OpenMP::OpenMP_CXX)
else()
  message(WARNING "OpenMP not found, skipping OpenMP benchmarks")
endif()

find_package(Taskflow QUIET)

if(Taskflow_FOUND)
  file(GLOB_RECURSE BENCH_Taskflow CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/source/**/taskflow.cpp"
  )
  target_sources(benchmark PRIVATE ${BENCH_Taskflow})
  target_link_libraries(benchmark PRIVATE Taskflow::Taskflow)
else()
  message(WARNING "Taskflow not found, skipping Taskflow benchmarks")
endif()

find_package(concurrencpp QUIET)

if(concurrencpp_FOUND)
  file(GLOB_RECURSE BENCH_concurrencpp CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/source/**/ccpp.cpp"
  )
  target_sources(benchmark PRIVATE ${BENCH_concurrencpp})
  target_link_libraries(benchmark PRIVATE concurrencpp::concurrencpp)
else()
  message(WARNING "concurrencpp not found, skipping concurrencpp benchmarks")
endif()

# TMC

include(FetchContent)

FetchContent_Declare(
  tmc
  GIT_REPOSITORY https://github.com/tzcnt/TooManyCooks.git
  GIT_TAG df81ef9f493e88df6b7ac08f0490e968866fa10b
  CONFIGURE_COMMAND "" BUILD_COMMAND ""
)

FetchContent_GetProperties(tmc)

if(NOT tmc_POPULATED)
  FetchContent_Populate(tmc)
endif()

target_compile_definitions(benchmark PRIVATE TMC_WORK_ITEM=CORO)
target_compile_definitions(benchmark PRIVATE TMC_PRIORITY_COUNT=1)
target_compile_definitions(benchmark PRIVATE TMC_USE_HWLOC)
target_compile_definitions(benchmark PRIVATE TMC_TRIVIAL_TASK)

target_include_directories(benchmark PRIVATE ${tmc_SOURCE_DIR}/include)

file(GLOB_RECURSE BENCH_tmc CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/**/tmc.cpp")

target_sources(benchmark PRIVATE ${BENCH_tmc} ${CMAKE_CURRENT_SOURCE_DIR}/source/tmc.cpp)

# ---- Coro benchmarks ----

add_executable(coro_bench ${CMAKE_CURRENT_SOURCE_DIR}/source/coroutine/incremental.cpp)

target_link_libraries(coro_bench PRIVATE libfork::libfork benchmark::benchmark_main)

# ---- End-of-file commands ----
add_folders(benchmarks)
