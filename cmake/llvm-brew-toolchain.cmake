cmake_minimum_required(VERSION 3.28)

find_program(BREW_EXE brew)

if(NOT BREW_EXE)
  message(FATAL_ERROR "Could not find 'brew' executable. Please install Homebrew.")
endif()

# Get LLVM prefix
execute_process(
  COMMAND ${BREW_EXE} --prefix llvm
  OUTPUT_VARIABLE LLVM_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY
)

set(CMAKE_C_COMPILER "${LLVM_PREFIX}/bin/clang")
set(CMAKE_CXX_COMPILER "${LLVM_PREFIX}/bin/clang++")
set(CMAKE_PREFIX_PATH "${LLVM_PREFIX}")

# Dynamically find the standard library modules JSON
# We look for the file pattern to handle version differences (e.g. llvm@21, llvm@19)
file(GLOB_RECURSE LIBCXX_MODULES_JSON "${LLVM_PREFIX}/lib/c++/libc++.modules.json")

if(LIBCXX_MODULES_JSON)
  set(CMAKE_CXX_STDLIB_MODULES_JSON "${LIBCXX_MODULES_JSON}")
else()
  # Fallback or specific version search if needed. 
  # For now, let's warn if not found, though newer LLVMs should have it.
  message(WARNING "Could not automatically find libc++.modules.json in ${LLVM_PREFIX}")
endif()

# Get macOS SDK path
execute_process(
  COMMAND xcrun --show-sdk-path
  OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY
)
