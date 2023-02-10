cmake_minimum_required(VERSION 3.14)

project(ForkpoolTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)
include(../cmake/windows-set-path.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(Forkpool REQUIRED)
  enable_testing()
endif()

# ---- Tests ----

add_executable(Forkpool_test source/Forkpool_test.cpp)
target_link_libraries(Forkpool_test PRIVATE Forkpool::Forkpool)
target_compile_features(Forkpool_test PRIVATE cxx_std_20)

add_test(NAME Forkpool_test COMMAND Forkpool_test)

# ---- End-of-file commands ----

add_folders(Test)
