cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# ---- Project ----

project(RiftenCoopTest)

# ---- Options ----

option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Add dependencies ----

find_package(Threads)

include(../cmake/CPM.cmake)

CPMAddPackage("gh:onqtam/doctest#2.4.5")

if(TEST_INSTALLED_VERSION)
  find_package(RiftenCoop REQUIRED)
else()
  CPMAddPackage(NAME RiftenCoop SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

# ---- Tests ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp)
add_executable(${PROJECT_NAME} "${sources}")

# Link dependencies
target_link_libraries(
  ${PROJECT_NAME} PUBLIC doctest::doctest RiftenCoop::RiftenCoop ${CMAKE_THREAD_LIBS_INIT}
)

# enable_testing()

# include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

# doctest_discover_tests(${PROJECT_NAME})
