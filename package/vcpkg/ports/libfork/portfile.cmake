vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO conorwilliams/libfork
    REF 2a4f7c9b09f201d438b85a2f6fbc1840fa9aa005
    # REF "v${VERSION}"
    SHA512 cdd7808a6f5b533148fcdf012710d0ddfa3af2374dc5c965ef364553f1a07894a195b1fc1f4c151da17bfcce5c327b4f0680ae2c82845fb1d2b34eb3dd059e36
    HEAD_REF cmake-sample-lib
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME "libfork")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

file(INSTALL "${SOURCE_PATH}/LICENSE.md" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

configure_file("${CMAKE_CURRENT_LIST_DIR}/usage.cmake" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" COPYONLY)