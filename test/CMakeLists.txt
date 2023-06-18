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

# ---- Tests ----

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

add_executable(libfork_test ${TEST_SOURCES})
target_link_libraries(libfork_test PRIVATE libfork::libfork Catch2::Catch2WithMain)
target_compile_features(libfork_test PRIVATE cxx_std_20)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

include(Catch)

catch_discover_tests(libfork_test)

# bench

# ---- End-of-file commands ----

add_folders(Test)
