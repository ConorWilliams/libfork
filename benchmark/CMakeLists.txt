cmake_minimum_required(VERSION 3.25) 

project(linforkBenchmark LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Set cmake to release ----


if(NOT CMAKE_BUILD_TYPE STREQUAL Release)
  message(WARNING "CMAKE_BUILD_TYPE is set to ${CMAKE_BUILD_TYPE}. This is not recommended for benchmarking.")
endif()

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(libfork REQUIRED)
endif()


include(FetchContent)

FetchContent_Declare(
    nanobench
    GIT_REPOSITORY https://github.com/martinus/nanobench.git
    GIT_TAG v4.1.0
    GIT_SHALLOW TRUE
    SYSTEM TRUE
)

FetchContent_MakeAvailable(nanobench)


# find_package(OpenMP REQUIRED)

# ---- Tests ----

add_executable(libforkBench source/example.cpp)

target_link_libraries(libforkBench PRIVATE nanobench)

add_custom_target(benchmark
  COMMAND libforkBench
  DEPENDS libforkBench
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


# ---- End-of-file commands ----

add_folders(Benchamark)


