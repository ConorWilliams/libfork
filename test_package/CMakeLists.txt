cmake_minimum_required(VERSION 3.15)
project(libfork_package_test CXX)

find_package(libfork REQUIRED)

add_executable(example src/example.cpp)
target_link_libraries(example PRIVATE libfork::libfork)