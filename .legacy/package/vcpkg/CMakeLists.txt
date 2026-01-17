cmake_minimum_required(VERSION 3.14)

project(libfork_test_vcpkg_port)

find_package(libfork CONFIG REQUIRED)

add_executable(HelloWorld source/hello_world.cpp)

target_link_libraries(HelloWorld PRIVATE libfork::libfork)