# Read version from a file and set version_major, version_minor, version_patch.
function(read_version fname)
  file(READ ${fname} ver)

  string(REGEX MATCH "#define [A-Z_0-9]*VERSION_MAJOR ([0-9]*)" _ ${ver})
  message(STATUS "Found ${fname} majour version: ${CMAKE_MATCH_1}")
  set(version_major ${CMAKE_MATCH_1} PARENT_SCOPE)

  string(REGEX MATCH "#define [A-Z_0-9]*VERSION_MINOR ([0-9]*)" _ ${ver})
  message(STATUS "Found ${fname}  minor version: ${CMAKE_MATCH_1}")
  set(version_minor ${CMAKE_MATCH_1} PARENT_SCOPE)

  string(REGEX MATCH "#define [A-Z_0-9]*VERSION_PATCH ([0-9]*)" _ ${ver})
  message(STATUS "Found ${fname}  patch version: ${CMAKE_MATCH_1}")
  set(version_patch ${CMAKE_MATCH_1} PARENT_SCOPE)
endfunction()
