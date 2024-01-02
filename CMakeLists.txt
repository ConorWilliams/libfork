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

#
option(LF_NO_HWLOC "Explicitly disable hwloc support" OFF)

if (NOT LF_NO_HWLOC)
  # hwloc has no cmake support so we have to do things manually
  find_path(HWLOC_hwloc_include hwloc.h)
  find_library(HWLOC_hwloc_library hwloc)

  if(HWLOC_hwloc_include AND HWLOC_hwloc_library)
    
    message(STATUS "Found hwloc headers: ${HWLOC_hwloc_include}")
    message(STATUS "Found hwloc library: ${HWLOC_hwloc_library}")

    # Instucts libfork to use hwloc.
    target_compile_definitions(libfork_libfork INTERFACE LF_USE_HWLOC)

    # Build a imported dummy library.
    add_library(hwloc_hwloc UNKNOWN IMPORTED)
    add_library(hwloc::hwloc ALIAS hwloc_hwloc)

    set_target_properties(hwloc_hwloc PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${HWLOC_hwloc_include}"
      IMPORTED_LOCATION "${HWLOC_hwloc_library}"
    )
    
    # Link dummy to libfork.
    target_link_libraries(libfork_libfork INTERFACE hwloc::hwloc)

  else()
    message(WARNING "hwloc.h not found, disabling libfork's NUMA support!")
  endif()
else()
  message(WARNING "Not looking for hwloc, NUMA support disabled!")
endif()

# ---- Boost (for clang acceleration) ---- 

find_package(Boost 1.83.0 COMPONENTS atomic)

if(Boost_FOUND)
  # Instucts libfork to use boost atomic (clang only)
  target_compile_definitions(libfork_libfork INTERFACE LF_USE_BOOST_ATOMIC)
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
