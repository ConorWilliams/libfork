cmake_minimum_required(VERSION 3.14)

project(libforkTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(libfork REQUIRED)
  enable_testing()
endif()

find_package(Catch2 REQUIRED)

# ---- Stack tracing ----

# include(FetchContent)

# # Also requires one of: libbfd (gnu binutils), libdwarf, libdw (elfutils) FetchContent_Declare(
# backward GIT_REPOSITORY https://github.com/conorwilliams/backward-cpp GIT_TAG
# 8cb73c397f38b0a375d49335e17980b6c3345ca1 )

# FetchContent_MakeAvailable(backward)

# ---- Tests ----

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

add_executable(libfork_test ${TEST_SOURCES} ${BACKWARD_ENABLE})
target_link_libraries(libfork_test PRIVATE libfork::libfork Catch2::Catch2WithMain)
target_compile_features(libfork_test PRIVATE cxx_std_23)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

include(Catch)

catch_discover_tests(libfork_test)

# bench

# ---- End-of-file commands ----

add_folders(Test)
