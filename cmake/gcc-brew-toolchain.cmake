cmake_minimum_required(VERSION 4.2.1)

# Set up Homebrew GCC@15 toolchain for CMake

find_program(BREW_EXE brew)

if(NOT BREW_EXE)
  message(FATAL_ERROR "Could not find 'brew' executable. Please install Homebrew.")
endif()

# Get Ninja prefix
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
)

# Get GCC prefix
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
)

find_program(CMAKE_CXX_COMPILER
  NAMES g++-15
  HINTS "${GCC_PREFIX}/bin"
  NO_DEFAULT_PATH
)

if(NOT CMAKE_C_COMPILER)
  message(FATAL_ERROR "Could not find gcc executable in ${GCC_PREFIX}/bin")
endif()

if(NOT CMAKE_CXX_COMPILER)
  message(FATAL_ERROR "Could not find g++ executable in ${GCC_PREFIX}/bin")
endif()

# Get macOS SDK path (only on macOS)
if(APPLE)
  execute_process(
    COMMAND xcrun --show-sdk-path
    OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
  )
endif()
