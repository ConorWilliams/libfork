cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)
include(cmake/read_version.cmake)

read_version(${CMAKE_CURRENT_SOURCE_DIR}/include/libfork/macro.hpp)

project(
  libfork
  VERSION ${version_major}.${version_minor}.${version_patch}
  DESCRIPTION "A C++20 coroutine library for fork-join parallelism"
  HOMEPAGE_URL "https://github.com/ConorWilliams/libfork"
  LANGUAGES CXX C
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

message(STATUS "CMAKE_BUILD_TYPE is set to '${CMAKE_BUILD_TYPE}'")

# ---- Dependencies ----

find_package(Threads REQUIRED)

find_package(hwloc CONFIG)

if(hwloc_FOUND)
  message(FATAL "hwloc has native cmake!")
else()
  find_package(HWLOC)
endif()

# ---- Declare library ----

add_library(libfork_libfork INTERFACE)
add_library(libfork::libfork ALIAS libfork_libfork)

target_link_libraries(libfork_libfork INTERFACE Threads::Threads)

set_property(TARGET libfork_libfork PROPERTY EXPORT_NAME libfork)

target_include_directories(
  libfork_libfork ${warning_guard} INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(libfork_libfork INTERFACE cxx_std_23)

if(HWLOC_FOUND)
  target_compile_definitions(libfork_libfork INTERFACE LF_HAS_HWLOC)
else()
  message(WARNING "Disabling libfork's hwloc support because we couldn't find it")
endif()

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT libfork_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(AUTHOR_WARNING "Developer mode is intended for developers of libfork")
endif()

include(cmake/dev-mode.cmake)
