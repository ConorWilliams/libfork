cmake_minimum_required(VERSION 4.2.1)

# Set up Homebrew GCC@15 & Ninja toolchain for CMake

find_program(BREW_EXE brew)

if(NOT BREW_EXE)
  message(FATAL_ERROR "Could not find 'brew' executable. Please install Homebrew.")
endif()

# --- Ninja

execute_process(
  COMMAND ${BREW_EXE} --prefix ninja
  OUTPUT_VARIABLE NINJA_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY
)

find_program(CMAKE_MAKE_PROGRAM
  NAMES ninja
  HINTS "${NINJA_PREFIX}/bin"
  NO_DEFAULT_PATH
  REQUIRED
)

# --- GCC

execute_process(
  COMMAND ${BREW_EXE} --prefix gcc
  OUTPUT_VARIABLE GCC_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY
)

find_program(CMAKE_C_COMPILER
  NAMES gcc-15
  HINTS "${GCC_PREFIX}/bin"
  NO_DEFAULT_PATH
  REQUIRED
)

find_program(CMAKE_CXX_COMPILER
  NAMES g++-15
  HINTS "${GCC_PREFIX}/bin"
  NO_DEFAULT_PATH
  REQUIRED
)

find_program(CMAKE_AR
  NAMES gcc-ar-15
  HINTS "${GCC_PREFIX}/bin"
  NO_DEFAULT_PATH
  REQUIRED
)

find_program(CMAKE_RANLIB
  NAMES gcc-ranlib-15
  HINTS "${GCC_PREFIX}/bin"
  NO_DEFAULT_PATH
  REQUIRED
)

find_program(CMAKE_NM
  NAMES gcc-nm-15
  HINTS "${GCC_PREFIX}/bin"
  NO_DEFAULT_PATH
  REQUIRED
)

# --- Binutils

execute_process(
  COMMAND ${BREW_EXE} --prefix binutils
  OUTPUT_VARIABLE BINUTILS_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -B${BINUTILS_PREFIX}/bin/" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -B${BINUTILS_PREFIX}/bin/" CACHE STRING "" FORCE)


# Get macOS SDK path (only on macOS)
if(APPLE)
  execute_process(
    COMMAND xcrun --show-sdk-path
    OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
  )
endif()
