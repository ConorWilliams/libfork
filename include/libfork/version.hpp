

#ifndef LF_VERSION_HPP
#define LF_VERSION_HPP

/**
 * @brief __[public]__ The major version of libfork.
 *
 * Increments with incompatible API changes/breaks.
 */
#define LF_VERSION_MAJOR 4

/**
 * @brief __[public]__ The minor version of libfork.
 *
 * Increments when functionality is added in an API backward compatible manner.
 *
 * Zero indicates an unreleased/development version which is unstable.
 */
#define LF_VERSION_MINOR 0

/** @brief __[public]__ The patch version of libfork.
 *
 * Increments when bug fixes are made in an API backward compatible manner.
 *
 * If the minor version is zero, this may be incremented as new features minor
 * ABI breaks are made.
 */
#define LF_VERSION_PATCH 0

#endif
