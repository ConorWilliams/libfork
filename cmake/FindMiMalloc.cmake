# * Try to find mimalloc headers and libraries.
#
# Usage of this module as follows:
#
# find_package(MiMalloc)
#
# Variables used by this module, they can change the default behavior and need to be set before
# calling find_package:
#
# MIMALLOC_ROOT_DIR Set this variable to the root installation of mimalloc if the module has
# problems finding the proper installation path.
#
# Variables defined by this module:
#
# MIMALLOC_FOUND            System has mimalloc libs/headers
#
# MIMALLOC_LIBRARY          The mimalloc library/libraries
#
# MIMALLOC_INCLUDE_DIR      The location of mimalloc headers

find_path(MIMALLOC_INCLUDE_DIRS NAMES mimalloc.h)

find_library(MIMALLOC_LIBRARY NAMES mimalloc)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(MiMalloc DEFAULT_MSG MIMALLOC_LIBRARY MIMALLOC_INCLUDE_DIRS)

mark_as_advanced(MIMALLOC_ROOT_DIR MIMALLOC_LIBRARY MIMALLOC_INCLUDE_DIRS)

message(STATUS "MiMalloc found: ${MIMALLOC_FOUND}")
message(STATUS "MiMalloc include dir:${MIMALLOC_INCLUDE_DIRS}")
message(STATUS "MiMalloc libraries: ${MIMALLOC_LIBRARY}")
