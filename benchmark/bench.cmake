cmake_minimum_required(VERSION 3.14) 

# ---- Warn if cmake is not release ----

if(NOT CMAKE_BUILD_TYPE STREQUAL Release)
  message(WARNING "CMAKE_BUILD_TYPE is set to '${CMAKE_BUILD_TYPE}'; 'Release' is recommended for benchmarking.")
endif()

add_subdirectory(benchmark/libfork)

find_package(OpenMP)

if(OpenMP_CXX_FOUND)
  add_subdirectory(benchmark/omp)
else()
  message(WARNING "OpenMP not found; OpenMP benchmarks will not be built.")
endif()



# # ---- Dependencies ----

# if(PROJECT_IS_TOP_LEVEL)
#   find_package(libfork REQUIRED)
# endif()

# find_package(OpenMP REQUIRED)
# find_package(TBB REQUIRED)

# CPMAddPackage("gh:martinus/nanobench#v4.1.0")

# # ---- Tests ----

# add_executable(libforkBench 
#   source/main.cpp 
#   source/fib.cpp 
#   source/reduce.cpp
# )

# target_link_libraries(libforkBench PRIVATE 
#   libfork::libfork 
#   nanobench 
#   OpenMP::OpenMP_CXX 
#   TBB::tbb
# )

# add_custom_target(benchmark
#   COMMAND libforkBench
#   DEPENDS libforkBench
#   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
# )


# ---- End-of-file commands ----

add_folders(Benchamark)


