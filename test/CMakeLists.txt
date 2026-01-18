cmake_minimum_required(VERSION 4.2.1 FATAL_ERROR)

project(libfork_test LANGUAGES CXX)

add_executable(main)

target_sources(main PRIVATE src/main.cpp)

target_link_libraries(main PRIVATE libfork::libfork)

add_test(NAME main COMMAND main)
