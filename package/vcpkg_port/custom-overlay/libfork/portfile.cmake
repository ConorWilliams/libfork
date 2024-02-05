vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO conorwilliams/libfork
    REF fa60c93c8e6e3cfa7d7d7e9d89c359de1299a0b1
    # REF "v${VERSION}"
    SHA512 6348660ba1ec801d69bb65f7bc61e009297accfa8cc33dd2385ce2b02e2b0b989efbd45cfb02a75c623b3db43e25579de8601cc8e9495754e7a9d654113902fb
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