cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)
include(cmake/prelude.cmake)
include(cmake/read_version.cmake)

read_version(${CMAKE_CURRENT_SOURCE_DIR}/include/libfork/core/macro.hpp)

project(
  libfork
  VERSION ${version_major}.${version_minor}.${version_patch}
  DESCRIPTION "A C++20 coroutine library for fork-join parallelism"
  HOMEPAGE_URL "https://github.com/ConorWilliams/libfork"
  LANGUAGES CXX C
)

message(STATUS "CMAKE_BUILD_TYPE is set to '${CMAKE_BUILD_TYPE}'")

# ---- Dependencies ----

find_package(Threads REQUIRED)

# ---- Declare library ----

add_library(libfork_libfork INTERFACE)
add_library(libfork::libfork ALIAS libfork_libfork)

target_link_libraries(libfork_libfork INTERFACE Threads::Threads)

set_property(TARGET libfork_libfork PROPERTY EXPORT_NAME libfork)

target_include_directories(
  libfork_libfork ${warning_guard} INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(libfork_libfork INTERFACE cxx_std_23)

# ---- Compiler options ----- #

option(LF_ASYNC_STACK_SIZE "The size of the async stack (in multiples 4kb pages)" OFF)

if(LF_ASYNC_STACK_SIZE)
  target_compile_definitions(libfork_libfork INTERFACE LF_ASYNC_STACK_SIZE=${LF_ASYNC_STACK_SIZE})
endif()

# If this is off then libfork will store a pointer to avoid any UB, enable only as an optimization
# if you know the compiler and are sure it is safe.
option(LF_COROUTINE_OFFSET "The ABI offset between a coroutine's promise and its resume member" OFF)

if(LF_COROUTINE_OFFSET)
  target_compile_definitions(libfork_libfork INTERFACE LF_COROUTINE_OFFSET=${LF_COROUTINE_OFFSET})
endif()

# ---- hwloc support ----

find_package(HWLOC)

if(HWLOC_FOUND)
  target_compile_definitions(libfork_libfork INTERFACE LF_HAS_HWLOC)
  target_include_directories(libfork_libfork SYSTEM INTERFACE ${HWLOC_INCLUDE_DIRS})
  target_link_libraries(libfork_libfork INTERFACE ${HWLOC_LIBRARIES})
else()
  message(WARNING "Disabling libfork's hwloc support because we couldn't find it")
endif()

# ---- Optional boost for clang acceleration ---- 

find_package(Boost 1.83.0 COMPONENTS atomic)

if(Boost_FOUND)
  target_link_libraries(libfork_libfork INTERFACE Boost::atomic)
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
