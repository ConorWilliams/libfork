cmake_minimum_required(VERSION 3.14)

# ---- Warn if cmake is not release ----
if(NOT CMAKE_BUILD_TYPE STREQUAL Release)
  message(
    WARNING "CMAKE_BUILD_TYPE is set to '${CMAKE_BUILD_TYPE}' but, 'Release' is recommended for benchmarking."
  )
endif()

add_subdirectory(bench)

# ---- End-of-file commands ----
add_folders(bench)
