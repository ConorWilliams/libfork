export module libfork.utils:exception;

import std;

namespace lf {

/**
 * @brief Base class for all libfork exceptions.
 */
export struct libfork_exception : std::exception {};

} // namespace lf
