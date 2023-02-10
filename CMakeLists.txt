cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    forkpool
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES NONE
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(forkpool_forkpool INTERFACE)
add_library(forkpool::forkpool ALIAS forkpool_forkpool)

set_property(
    TARGET forkpool_forkpool PROPERTY
    EXPORT_NAME forkpool
)

target_include_directories(
    forkpool_forkpool ${warning_guard}
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(forkpool_forkpool INTERFACE cxx_std_20)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT forkpool_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of forkpool"
  )
endif()

include(cmake/dev-mode.cmake)
