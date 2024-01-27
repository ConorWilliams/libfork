cmake_minimum_required(VERSION 3.15)
project(libfork_package_test CXX)

# TODO: remove these lines
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../output")
message("current list dir = ${CMAKsE_CURRENT_LIST_DIR}, prefix path = ${CMAKE_PREFIX_PATH}, libfork_DIR=${libfork_DIR}")

find_package(libfork REQUIRED)

add_executable(example src/example.cpp)
target_link_libraries(example PRIVATE libfork::libfork)