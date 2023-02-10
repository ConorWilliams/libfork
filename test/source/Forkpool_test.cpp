#include "Forkpool/Forkpool.hpp"

auto main() -> int
{
  auto const result = name();

  return result == "Forkpool" ? 0 : 1;
}
