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

find_package(OpenMP REQUIRED)

find_package(TBB REQUIRED)

include(FetchContent)

FetchContent_Declare(
    nanobench
    GIT_REPOSITORY https://github.com/martinus/nanobench.git
    GIT_TAG v4.1.0
    GIT_SHALLOW TRUE
    SYSTEM TRUE
)

FetchContent_MakeAvailable(nanobench)

include(FetchContent)

# Also requires one of: libbfd (gnu binutils), libdwarf, libdw (elfutils)
FetchContent_Declare(
    backward
    GIT_REPOSITORY https://github.com/bombela/backward-cpp
    GIT_TAG v1.6
    GIT_SHALLOW TRUE
    SYSTEM TRUE
)
        
FetchContent_MakeAvailable(backward)



# ---- Tests ----

add_executable(libforkBench source/main.cpp source/fib.cpp ${BACKWARD_ENABLE})

add_backward(libforkBench)

target_link_libraries(libforkBench PRIVATE libfork::libfork nanobench OpenMP::OpenMP_CXX TBB::tbb)

add_custom_target(benchmark
  COMMAND libforkBench
  DEPENDS libforkBench
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


# ---- End-of-file commands ----

add_folders(Benchamark)


