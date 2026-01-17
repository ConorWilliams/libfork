add_executable(main)

target_sources(main PRIVATE main.cxx)

target_link_libraries(main PRIVATE libfork)

add_test(NAME main COMMAND main)