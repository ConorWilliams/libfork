#include "forkpool/forkpool.hpp"

auto main() -> int
{
  auto const result = name();

  return result == "forkpool" ? 0 : 1;
}
