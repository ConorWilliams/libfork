add_executable(main)

target_sources(main PRIVATE src/main.cpp)

target_link_libraries(main PRIVATE libfork::libfork)

add_test(NAME main COMMAND main)
