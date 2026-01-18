cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(libfork_test LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)

# --------- Tests ---------

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(libfork_test ${TEST_SOURCES})
target_link_libraries(libfork_test PRIVATE libfork::libfork Catch2::Catch2WithMain)
target_compile_features(libfork_test PRIVATE cxx_std_23)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

include(Catch)

catch_discover_tests(libfork_test)
