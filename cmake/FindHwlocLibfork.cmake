# FindHwlocLibfork
# ----------
#
# Try to find Portable Hardware Locality (hwloc) libraries.
# https://www.open-mpi.org/software/hwloc
#
# You may declare HWLOC_ROOT environment variable to tell where
# your hwloc library is installed. 
#
# Once done this will define::
#
#   HwlocLibfork_FOUND            - True if hwloc was found
#   HwlocLibfork_INCLUDE_DIRS     - Include directories for hwloc
#   HwlocLibfork_LINK_LIBRARIES   - Link against these libraries to use hwloc
#   HwlocLibfork_VERSION          - Version
#

#=============================================================================
# Copyright 2014 Mikael Lepistö, 2024 Conor J. Williams
#
# Distributed under the OSI-approved BSD License (the "License");
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

if(WIN32)
  
  find_path(HwlocLibfork_INCLUDE_DIR
    NAMES
      hwloc.h
    PATHS
      ENV "PROGRAMFILES(X86)"
      ENV HWLOC_ROOT
    PATH_SUFFIXES
      include
  )

  find_library(HwlocLibfork_LIBRARY
    NAMES
      hwloc
    PATHS
      ENV "PROGRAMFILES(X86)"
      ENV HWLOC_ROOT
    PATH_SUFFIXES
      lib
  )

  #
  # Check if the found library can be used to linking 
  #
  SET (_TEST_SOURCE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/linktest.c")

  FILE (WRITE "${_TEST_SOURCE}"
    "
    #include <hwloc.h>
    int main()
    { 
      hwloc_topology_t topology;
      int nbcores;
      hwloc_topology_init(&topology);
      hwloc_topology_load(topology);
      nbcores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
      hwloc_topology_destroy(topology);
      return 0;
    }
    "
  )

  TRY_COMPILE(_LINK_SUCCESS ${CMAKE_BINARY_DIR} "${_TEST_SOURCE}"
    CMAKE_FLAGS
    "-DINCLUDE_DIRECTORIES:STRING=${HwlocLibfork_INCLUDE_DIR}"
    CMAKE_FLAGS
    "-DLINK_LIBRARIES:STRING=${HwlocLibfork_LIBRARY}"
  )

  IF(NOT _LINK_SUCCESS)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      message(STATUS "You are building 64bit target.")
    ELSE()
      message(STATUS "You are building 32bit code. If you like to build x64 use e.g. -G 'Visual Studio 12 Win64' generator." )
    ENDIF()
    message(FATAL_ERROR "Library found, but linking test program failed.")
  ENDIF()

  #
  # Resolve version if some compiled binary found...
  #
  find_program(HWLOC_INFO_EXECUTABLE
    NAMES 
      hwloc-info
    PATHS
      ENV HWLOC_ROOT 
    PATH_SUFFIXES
      bin
  )
  
  if(HWLOC_INFO_EXECUTABLE)
    execute_process(
      COMMAND ${HWLOC_INFO_EXECUTABLE} "--version" 
      OUTPUT_VARIABLE HWLOC_VERSION_LINE 
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX MATCH "([0-9]+.[0-9]+)$" HwlocLibfork_VERSION "${HWLOC_VERSION_LINE}")
    unset(HWLOC_VERSION_LINE)
  endif()
  
  #
  # All good
  #

  set(HwlocLibfork_LIBRARIES ${HwlocLibfork_LIBRARY})
  set(HwlocLibfork_LINK_LIBRARIES ${HwlocLibfork_LIBRARY})
  set(HwlocLibfork_INCLUDE_DIRS ${HwlocLibfork_INCLUDE_DIR})

  include(FindPackageHandleStandardArgs)

  find_package_handle_standard_args(
    HwlocLibfork
    REQUIRED_VARS HwlocLibfork_LIBRARIES HwlocLibfork_INCLUDE_DIRS HwlocLibfork_LINK_LIBRARIES
    VERSION_VAR HwlocLibfork_VERSION
  )

  mark_as_advanced(HwlocLibfork_INCLUDE_DIR HwlocLibfork_LIBRARY HwlocLibfork_LINK_LIBRARIES)

else()

  find_package(PkgConfig)

  if(HWLOC_ROOT)
    set(ENV{PKG_CONFIG_PATH} "${HWLOC_ROOT}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
  else()
    foreach(PREFIX ${CMAKE_PREFIX_PATH})
      set(PKG_CONFIG_PATH "${PKG_CONFIG_PATH}:${PREFIX}/lib/pkgconfig")
    endforeach()
    set(ENV{PKG_CONFIG_PATH} "${PKG_CONFIG_PATH}:$ENV{PKG_CONFIG_PATH}")
  endif()

  if(hwloc_FIND_REQUIRED)
    set(_hwloc_OPTS "REQUIRED")
  elseif(hwloc_FIND_QUIETLY)
    set(_hwloc_OPTS "QUIET")
  else()
    set(_hwloc_output 1)
  endif()

  if(hwloc_FIND_VERSION)
    if(hwloc_FIND_VERSION_EXACT)
      pkg_check_modules(HwlocLibfork ${_hwloc_OPTS} hwloc=${hwloc_FIND_VERSION})
    else()
      pkg_check_modules(HwlocLibfork ${_hwloc_OPTS} hwloc>=${hwloc_FIND_VERSION})
    endif()
  else()
    pkg_check_modules(HwlocLibfork ${_hwloc_OPTS} hwloc)
  endif()

  if(HwlocLibfork_FOUND)
    string(REPLACE "." ";" HwlocLibfork_VERSION_PARSED "${HwlocLibfork_VERSION}")
    set(HwlocLibfork_VERSION "${HwlocLibfork_VERSION}" CACHE STRING "version of HwlocLibfork as a list")
    list(GET HwlocLibfork_VERSION_PARSED 0 HwlocLibfork_VERSION_MAJOR)
    set(HwlocLibfork_VERSION_MAJOR "${HwlocLibfork_VERSION_MAJOR}" CACHE STRING "Major version of HwlocLibfork")
    list(GET HwlocLibfork_VERSION_PARSED 1 HwlocLibfork_VERSION_MINOR)
    set(HwlocLibfork_VERSION_MINOR "${HwlocLibfork_VERSION_MINOR}" CACHE STRING "Minor version of HwlocLibfork")

    include(FindPackageHandleStandardArgs)

    find_package_handle_standard_args(
        HwlocLibfork
        REQUIRED_VARS HwlocLibfork_LIBRARIES HwlocLibfork_INCLUDE_DIRS HwlocLibfork_LINK_LIBRARIES
        VERSION_VAR HwlocLibfork_VERSION
    )

    if(NOT ${HwlocLibfork_VERSION} VERSION_LESS 1.7.0)
      set(HwlocLibfork_GL_FOUND 1)
    endif()

    if(_hwloc_output)
      message(STATUS "Found hwloc ${HwlocLibfork_VERSION} in ${HwlocLibfork_INCLUDE_DIRS}:${HwlocLibfork_LIBRARIES}")
    endif()
  endif()
endif()