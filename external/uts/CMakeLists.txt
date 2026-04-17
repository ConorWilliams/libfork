cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(uts_external LANGUAGES C)

add_library(uts_c OBJECT
  uts.c
  rng/brg_sha1.c
)

target_include_directories(uts_c PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/rng>
)
