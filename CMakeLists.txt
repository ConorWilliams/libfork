cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    Forkpool
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES NONE
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(Forkpool_Forkpool INTERFACE)
add_library(Forkpool::Forkpool ALIAS Forkpool_Forkpool)

set_property(
    TARGET Forkpool_Forkpool PROPERTY
    EXPORT_NAME Forkpool
)

target_include_directories(
    Forkpool_Forkpool ${warning_guard}
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(Forkpool_Forkpool INTERFACE cxx_std_20)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT Forkpool_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of Forkpool"
  )
endif()

include(cmake/dev-mode.cmake)
