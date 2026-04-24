cmake_minimum_required(VERSION 4.2.1)

# Set up Homebrew LLVM & Ninja toolchain for CMake

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

# --- LLVM

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
  REQUIRED
)

find_program(CMAKE_CXX_COMPILER
  NAMES clang++
  HINTS "${LLVM_PREFIX}/bin"
  NO_DEFAULT_PATH
  REQUIRED
)

find_program(CMAKE_AR
  NAMES llvm-ar
  HINTS "${LLVM_PREFIX}/bin"
  NO_DEFAULT_PATH
  REQUIRED
)

find_program(CMAKE_RANLIB
  NAMES llvm-ranlib
  HINTS "${LLVM_PREFIX}/bin"
  NO_DEFAULT_PATH
  REQUIRED
)

find_program(CMAKE_NM
  NAMES llvm-nm
  HINTS "${LLVM_PREFIX}/bin"
  NO_DEFAULT_PATH
  REQUIRED
)

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
