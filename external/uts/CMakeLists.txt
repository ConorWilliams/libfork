cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(uts_external LANGUAGES C)

add_library(uts_c OBJECT)

target_sources(uts_c
  PRIVATE
    src/uts.c
    src/rng/brg_sha1.c
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    FILES
      include/uts/uts.h
      include/uts/rng/rng.h
      include/uts/rng/brg_sha1.h
      include/uts/rng/brg_types.h
)

target_include_directories(uts_c PRIVATE include)
