cmake_minimum_required(VERSION 3.14)

project(libforkTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)
include(../cmake/windows-set-path.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(libfork REQUIRED)
  enable_testing()
endif()

# ---- Tests ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

foreach(source IN LISTS sources)
  get_filename_component(name ${source} NAME_WE)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE libfork::libfork)
  target_compile_features(${name} PRIVATE cxx_std_20)
  add_test(NAME ${name} COMMAND ${name})
endforeach()


# add_executable(libfork_test ${sources})
# target_link_libraries(libfork_test PRIVATE libfork::libfork)
# target_compile_features(libfork_test PRIVATE cxx_std_20)

# add_test(NAME libfork_test COMMAND libfork_test)


# ---- End-of-file commands ----

add_folders(Test)


