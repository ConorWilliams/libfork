#include <cstdio>

#include "libfork/__impl/exception.hpp"

import std;

namespace lf::impl {

[[noreturn]] void terminate_with(char const *message, char const *file, int line) noexcept {
  LF_TRY {
    std::println(stderr, "{}:{}: {}", file, line, message);
  } LF_CATCH_ALL {
    // Drop exceptions during termination
  }
  std::terminate();
}

} // namespace lf::impl
