cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/prelude.cmake)
include(cmake/read_version.cmake)

read_version(${CMAKE_CURRENT_SOURCE_DIR}/include/libfork/core/macro.hpp)

project(
  libfork
  VERSION ${version_major}.${version_minor}.${version_patch}
  DESCRIPTION
    "A bleeding-edge, lock-free, wait-free, continuation-stealing fork-join library built on C++20's coroutines."
  HOMEPAGE_URL "https://github.com/ConorWilliams/libfork"
  LANGUAGES CXX C
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

message(STATUS "CMAKE_BUILD_TYPE is set to '${CMAKE_BUILD_TYPE}'")

# ---- System dependencies ----

find_package(Threads REQUIRED)

# ------ Declare library ------

add_library(libfork_libfork INTERFACE)
add_library(libfork::libfork ALIAS libfork_libfork)

target_link_libraries(libfork_libfork INTERFACE Threads::Threads)

set_property(TARGET libfork_libfork PROPERTY EXPORT_NAME libfork)

target_include_directories(
  libfork_libfork ${warning_guard} INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(libfork_libfork INTERFACE cxx_std_23)

# ---- Compiler options ----- #

option(LF_FIBRE_INIT_SIZE "The initial size (bytes) of a fibre's stack (default 4 KiB)" OFF)

if(LF_FIBRE_INIT_SIZE)
  target_compile_definitions(libfork_libfork INTERFACE LF_FIBRE_INIT_SIZE=${LF_FIBRE_INIT_SIZE})
endif()

# If this is off then libfork will store a pointer to avoid any UB, enable only as an optimization
# if you know the compiler and are sure it is safe.
option(LF_COROUTINE_OFFSET "The ABI offset between a coroutine's promise and its resume member" OFF)

if(LF_COROUTINE_OFFSET)
  target_compile_definitions(libfork_libfork INTERFACE LF_COROUTINE_OFFSET=${LF_COROUTINE_OFFSET})
endif()

# --------------- Optional dependancies---------------

# ---------------- hwloc----------------

option(LF_NO_HWLOC "Disable hwloc support" OFF)

if(NOT LF_NO_HWLOC)
  # Conan exports a proper target for hwloc, vcpkg does not.
  find_package(hwloc 2.0 QUIET)

  if(hwloc_FOUND)
    # Instructs libfork to use hwloc.
    target_compile_definitions(libfork_libfork INTERFACE LF_USE_HWLOC)
    # Link to exported target.
    target_link_libraries(libfork_libfork INTERFACE hwloc::hwloc)
    # Let user know version
    message(STATUS "Found HWLOC version ${hwloc_VERSION}, NUMA support enabled!")
  else()

    find_package(PkgConfig)

    if(PKG_CONFIG_EXECUTABLE)

      unset(HWLOC_FOUND CACHE) # Prevent dirty cache

      pkg_search_module(HWLOC QUIET IMPORTED_TARGET hwloc>=2.0)

      if(HWLOC_FOUND)
        # Instructs libfork to use hwloc.
        target_compile_definitions(libfork_libfork INTERFACE LF_USE_HWLOC)
        # Link to imported target.
        target_link_libraries(libfork_libfork INTERFACE PkgConfig::HWLOC)
        # Let user know version
        message(STATUS "Found HWLOC version ${HWLOC_VERSION}, NUMA support enabled!")
      else()
        message(WARNING "HWLOC: not found, NUMA support disabled!")
      endif()
    else()
      message(WARNING "PKG_CONFIG_EXECUTABLE: not found, NUMA support disabled!")
    endif()
  endif()
endif()

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer options ----

if(NOT libfork_DEV_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(AUTHOR_WARNING "Developer mode is intended for developers of libfork")
endif()

include(cmake/dev-mode.cmake)
