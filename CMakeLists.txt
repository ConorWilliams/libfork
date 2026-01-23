cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

# See `Help/dev/experimental.rst`
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")

include(cmake/read_version.cmake)

read_version(${CMAKE_CURRENT_SOURCE_DIR}/include/libfork/version.hpp)

project(
  libfork
  VERSION ${version_major}.${version_minor}.${version_patch}
  LANGUAGES CXX
)

# ---- Project options ----

option(libfork_DEV_MODE "Enable developer build (tests/benchmarks/etc) for libfork" OFF)

# ---- System dependencies ----

find_package(Threads REQUIRED)

# ===========================

# Tell CMake that we explicitly want `import std`. This will initialize the
# property on all targets declared after this to 1
# TODO: set property per target
set(CMAKE_CXX_MODULE_STD 1)

add_library(libfork_libfork)
add_library(libfork::libfork ALIAS libfork_libfork)

# target_link_libraries(libfork_libfork PRIVATE Threads::Threads)

set_property(TARGET libfork_libfork PROPERTY EXPORT_NAME libfork)

target_compile_features(libfork_libfork PUBLIC cxx_std_23)

# Public headers
target_sources(libfork_libfork
  PUBLIC
    FILE_SET HEADERS FILES
      include/libfork/version.hpp
    BASE_DIRS
      include
)

# Add the module files to the library, must be public because
# consumers will need bo build the BMI
target_sources(libfork_libfork
  PUBLIC
    FILE_SET CXX_MODULES FILES
      src/core/core.cxx
      src/core/promise.cxx
    PRIVATE
      # src/libfork.cpp
)

# ======================

if(libfork_DEV_MODE)

  include(CTest) # Enables the BUILD_TESTING option

  if(BUILD_TESTING)
    add_subdirectory(test)
  endif()

  add_subdirectory(benchmark)

endif()

# TODO: --benchmark_dry_run


# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#
# include(cmake/prelude.cmake)
#
# include(cmake/project-is-top-level.cmake)
# include(cmake/variables.cmake)
#
# message(STATUS "CMAKE_BUILD_TYPE is set to '${CMAKE_BUILD_TYPE}'")
#
#
# # ------ Declare library ------
#
# add_library(libfork_libfork INTERFACE)
# add_library(libfork::libfork ALIAS libfork_libfork)
#
# target_link_libraries(libfork_libfork INTERFACE Threads::Threads)
#
# set_property(TARGET libfork_libfork PROPERTY EXPORT_NAME libfork)
#
# target_include_directories(
#   libfork_libfork ${warning_guard} INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
# )
#
# target_compile_features(libfork_libfork INTERFACE cxx_std_23)
#
# # ---- Compiler options ----- #
#
# option(LF_FIBRE_INIT_SIZE "The initial size (bytes) of a fibre's stack (default 4 KiB)" OFF)
#
# if(LF_FIBRE_INIT_SIZE)
#   target_compile_definitions(libfork_libfork INTERFACE LF_FIBRE_INIT_SIZE=${LF_FIBRE_INIT_SIZE})
# endif()
#
# # If this is off then libfork will store a pointer to avoid any UB, enable only as an optimization
# # if you know the compiler and are sure it is safe.
# option(LF_COROUTINE_OFFSET "The ABI offset between a coroutine's promise and its resume member" OFF)
#
# if(LF_COROUTINE_OFFSET)
#   target_compile_definitions(libfork_libfork INTERFACE LF_COROUTINE_OFFSET=${LF_COROUTINE_OFFSET})
# endif()
#
# # --------------- Optional dependancies---------------
#
# # ---------------- hwloc----------------
#
# option(LF_NO_HWLOC "Disable hwloc support" OFF)
#
# if(NOT LF_NO_HWLOC)
#   # Conan exports a proper target for hwloc, vcpkg does not.
#   find_package(hwloc QUIET)
#
#   if(hwloc_FOUND)
#     # Instructs libfork to use hwloc.
#     target_compile_definitions(libfork_libfork INTERFACE LF_USE_HWLOC)
#     # Link to exported target.
#     target_link_libraries(libfork_libfork INTERFACE hwloc::hwloc)
#     # Let user know version
#     message(STATUS "Found HWLOC version ${hwloc_VERSION}, NUMA support enabled!")
#   else()
#
#     find_package(PkgConfig)
#
#     if(PKG_CONFIG_EXECUTABLE)
#
#       unset(HWLOC_FOUND CACHE) # Prevent dirty cache
#
#       pkg_search_module(HWLOC QUIET IMPORTED_TARGET hwloc)
#
#       if(HWLOC_FOUND)
#         # Instructs libfork to use hwloc.
#         target_compile_definitions(libfork_libfork INTERFACE LF_USE_HWLOC)
#         # Link to imported target.
#         target_link_libraries(libfork_libfork INTERFACE PkgConfig::HWLOC)
#         # Let user know version
#         message(STATUS "Found HWLOC version ${HWLOC_VERSION}, NUMA support enabled!")
#       else()
#         message(WARNING "HWLOC: not found, NUMA support disabled!")
#       endif()
#     else()
#       message(WARNING "PKG_CONFIG_EXECUTABLE: not found, NUMA support disabled!")
#     endif()
#   endif()
# endif()
#
# # ---- Install rules ----
#
# if(NOT CMAKE_SKIP_INSTALL_RULES)
#   include(cmake/install-rules.cmake)
# endif()
#
# # ---- Developer options ----
#
# if(NOT libfork_DEV_MODE)
#   return()
# elseif(NOT PROJECT_IS_TOP_LEVEL)
#   message(AUTHOR_WARNING "Developer mode is intended for developers of libfork")
# endif()
#
# include(cmake/dev-mode.cmake)
