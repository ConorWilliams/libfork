include(cmake/folders.cmake)

include(CTest) # Enables the BUILD_TESTING option

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

option(BUILD_BENCHMARKS "Build benchmarks using nanobench" ${BUILD_TESTING})
if(BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif()

option(BUILD_DOCS "Build documentation using Doxygen and Sphinx" OFF)
if(BUILD_DOCS)
  add_subdirectory(docs)
endif()

option(ENABLE_COVERAGE "Enable coverage support separate from CTest's" OFF)
if(ENABLE_COVERAGE)
  include(cmake/coverage.cmake)
endif()

include(cmake/lint-targets.cmake)
include(cmake/spell-targets.cmake)

add_folders(Project)
