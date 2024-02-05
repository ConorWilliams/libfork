cmake_minimum_required(VERSION 3.15)

project(libfork_package_test CXX)

find_package(libfork CONFIG REQUIRED)

add_executable(hello_world source/hello_world.cpp)

target_link_libraries(hello_world PRIVATE libfork::libfork)