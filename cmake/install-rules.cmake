if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/libfork-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package libfork)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT libfork_Development
)

install(
    TARGETS libfork_libfork
    EXPORT libforkTargets
    RUNTIME #
    COMPONENT libfork_Runtime
    LIBRARY #
    COMPONENT libfork_Runtime
    NAMELINK_COMPONENT libfork_Development
    ARCHIVE #
    COMPONENT libfork_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    libfork_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE libfork_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(libfork_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${libfork_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT libfork_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${libfork_INSTALL_CMAKEDIR}"
    COMPONENT libfork_Development
)

install(
    EXPORT libforkTargets
    NAMESPACE libfork::
    DESTINATION "${libfork_INSTALL_CMAKEDIR}"
    COMPONENT libfork_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
