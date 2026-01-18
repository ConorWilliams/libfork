cmake_minimum_required(VERSION 4.2.1)

# Set up Homebrew LLVM & Ninja toolchain for CMake

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

# Get LLVM prefix
execute_process(
  COMMAND ${BREW_EXE} --prefix llvm
  OUTPUT_VARIABLE LLVM_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY
)

find_program(CMAKE_C_COMPILER
  NAMES clang
  HINTS "${LLVM_PREFIX}/bin"
  NO_DEFAULT_PATH
)

find_program(CMAKE_CXX_COMPILER
  NAMES clang++
  HINTS "${LLVM_PREFIX}/bin"
  NO_DEFAULT_PATH
)

if(NOT CMAKE_C_COMPILER)
  message(FATAL_ERROR "Could not find clang executable in ${LLVM_PREFIX}/bin")
endif()

if(NOT CMAKE_CXX_COMPILER)
  message(FATAL_ERROR "Could not find clang++ executable in ${LLVM_PREFIX}/bin")
endif()

# Dynamically find the standard library modules JSON, brew puts it in the wrong place
file(GLOB_RECURSE LIBCXX_MODULES_JSON "${LLVM_PREFIX}/lib/**/libc++.modules.json")

if(LIBCXX_MODULES_JSON)
  set(CMAKE_CXX_STDLIB_MODULES_JSON "${LIBCXX_MODULES_JSON}")
else()
  message(FATAL_ERROR "Could not automatically find libc++.modules.json in ${LLVM_PREFIX}")
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
