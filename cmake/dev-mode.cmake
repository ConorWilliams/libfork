include(cmake/folders.cmake)

include(CTest)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

option(BUILD_DOCS "Build documentation using Doxygen and m.css" OFF)
if(BUILD_DOCS)
  add_subdirectory(docs)
endif()


include(cmake/lint-targets.cmake)
include(cmake/spell-targets.cmake)

add_folders(Project)
