cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    libfork
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    libfork_libfork
    source/libfork.cpp
)
add_library(libfork::libfork ALIAS libfork_libfork)

include(GenerateExportHeader)
generate_export_header(
    libfork_libfork
    BASE_NAME libfork
    EXPORT_FILE_NAME export/libfork/libfork_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(libfork_libfork PUBLIC LIBFORK_STATIC_DEFINE)
endif()

set_target_properties(
    libfork_libfork PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME libfork
    OUTPUT_NAME libfork
)

target_include_directories(
    libfork_libfork ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    libfork_libfork SYSTEM
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(libfork_libfork PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(libfork_libfork PRIVATE fmt::fmt)

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
