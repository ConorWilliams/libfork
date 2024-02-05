vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO conorwilliams/libfork
    REF d38410a6f6b5e1cc575fba751cd7e3ad501c291c
    # REF "v${VERSION}"
    SHA512 2a6bbe10198747306a4fd299f98389feb1eacb355471ff30d59a524dea91fa60db2a05bb3cbac23c05efd1e7211aea5d13f8633bc6ebbc07b5b489e8f744d097
    HEAD_REF cmake-sample-lib
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME "libfork")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

file(INSTALL "${SOURCE_PATH}/LICENSE.md" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

configure_file("${CMAKE_CURRENT_LIST_DIR}/usage" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" COPYONLY)