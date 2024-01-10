#include <catch2/catch_test_macros.hpp>

#include "../../single_header/libfork.hpp"

namespace {

using lf::task;

using lf::call;
using lf::fork;
using lf::join;

inline constexpr auto fib = [](auto fib, int n) -> task<int> {
  if (n < 2) {
    co_return n;
  }

  int a, b;

  co_await fork(&a, fib)(n - 1);
  co_await call(&b, fib)(n - 2);

  co_await join;

  co_return a + b;
};

auto main() -> int {

  int fib_10 = lf::sync_wait(lf::lazy_pool{}, fib, 10);

  return fib_10 == 55 ? 0 : 1;
}

} // namespace

TEST_CASE("Single header", "[main]") { REQUIRE(main() == 0); }
