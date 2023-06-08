#include "libfork/libfork.hpp"

auto main() -> int
{
  auto const result = name();

  return result == "libfork" ? 0 : 1;
}
