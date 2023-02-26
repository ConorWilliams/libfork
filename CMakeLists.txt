cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
  libfork
  VERSION 2.0.0
  DESCRIPTION "A C++ library for fork-join parallelism using coroutines"
  HOMEPAGE_URL "https://github.com/ConorWilliams/libfork"
  LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Dependencies ----
find_package(Threads REQUIRED)

# ---- Declare library ----
add_library(libfork_libfork INTERFACE)
add_library(libfork::libfork ALIAS libfork_libfork)

target_link_libraries(libfork_libfork INTERFACE Threads::Threads)

set_property(
  TARGET libfork_libfork PROPERTY
  EXPORT_NAME libfork
)

target_include_directories(
  libfork_libfork ${warning_guard}
  INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(libfork_libfork INTERFACE cxx_std_20)

# ---- Install rules ----
if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----
if(NOT libfork_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
    AUTHOR_WARNING
    "Developer mode is intended for developers of libfork"
  )
endif()

include(cmake/dev-mode.cmake)
