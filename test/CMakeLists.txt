cmake_minimum_required(VERSION 3.14)

project(libforkTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)
include(../cmake/windows-set-path.cmake)
include(../cmake/CPM.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(libfork REQUIRED)
  enable_testing()
endif()

CPMAddPackage("gh:catchorg/Catch2#v3.3.2")

include(FetchContent)

# Also requires one of: libbfd (gnu binutils), libdwarf, libdw (elfutils)
FetchContent_Declare(
  backward
  GIT_REPOSITORY https://github.com/bombela/backward-cpp
  GIT_TAG v1.6
)
FetchContent_MakeAvailable(backward)

# ---- Tests ----

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/libfork.cpp")

add_executable(libfork_test ${TEST_SOURCES} ${BACKWARD_ENABLE})

add_backward(libfork_test)

target_link_libraries(libfork_test PRIVATE libfork::libfork)

target_compile_features(libfork_test PRIVATE cxx_std_20)

add_test(libfork_test libfork_test)

# list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

# include(Catch)

# catch_discover_tests(libfork_test)

# bench

# ---- End-of-file commands ----

add_folders(Test)
