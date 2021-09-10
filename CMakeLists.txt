cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# ---- Project ----

project(
  RiftenForkpool
  VERSION 0.1.0
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Add dependencies ----

find_package(Threads)

include(cmake/CPM.cmake)

CPMAddPackage("gh:ConorWilliams/ConcurrentDeque@2.0.0")
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.7.0")

# ---- Create library ----

add_library(${PROJECT_NAME} INTERFACE)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

target_compile_options(${PROJECT_NAME} INTERFACE -Wall -Wextra -Wpedantic)

# Link dependencies
target_link_libraries(
  ${PROJECT_NAME} INTERFACE ${CMAKE_THREAD_LIBS_INIT} RiftenDeque::RiftenDeque
)

target_include_directories(
  ${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Create an installable target ----

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES ""
)
