
#include <algorithm>                             // for min
#include <catch2/catch_template_test_macros.hpp> // for TEMPLATE_TEST_CASE, TypeList
#include <catch2/catch_test_macros.hpp>          // for operator<=, operator==, INTERNAL_CATCH_...
#include <concepts>                              // for constructible_from
#include <cstddef>                               // for size_t
#include <functional>                            // for plus, identity, multiplies
#include <iostream>
#include <limits>      // for numeric_limits
#include <numeric>     // for inclusive_scan
#include <random>      // for random_device, uniform_int_distribution
#include <string>      // for operator+, string, basic_string
#include <thread>      // for thread
#include <type_traits> // for type_identity
#include <utility>     // for forward
#include <vector>      // for operator==, vector

#include "libfork/algorithm/scan.hpp" // for scan
#include "libfork/core.hpp"           // for task, sync_wait
#include "libfork/schedule.hpp"       // for xoshiro, busy_pool, lazy_pool, unit_pool
#include "matrix.hpp"                 // for matrix, operator*

using namespace lf;

namespace {

template <typename T>
auto make_scheduler() -> T {
  if constexpr (std::constructible_from<T, std::size_t>) {
    return T{std::min(4U, std::thread::hardware_concurrency())};
  } else {
    return T{};
  }
}

auto random_vec(std::type_identity<int>, std::size_t n) -> std::vector<int> {

  std::vector<int> out(n);

  lf::xoshiro rng{lf::seed, std::random_device{}};
  std::uniform_int_distribution<int> dist{0, std::numeric_limits<int>::max() / (static_cast<int>(n) + 1) / 2};

  for (auto &&elem : out) {
    elem = dist(rng);
  }

  return out;
}

auto random_vec(std::type_identity<std::string>, std::size_t n) -> std::vector<std::string> {

  std::vector<std::string> out(n);

  lf::xoshiro rng{lf::seed, std::random_device{}};

  std::uniform_int_distribution<int> dist{'a', 'z'};

  for (auto &&elem : out) {
    elem.push_back(dist(rng));
  }

  return out;
}

template <typename T, typename Sch, typename Check, typename F, typename Proj = std::identity>
void test(Sch &&sch, F bop, Proj proj, Check check) {

  std::vector<std::size_t> ns = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  if (!std::same_as<T, std::string>) {
    ns.insert(ns.end(), {30, 1'000, 3'000, 10'000, 30'000, 100'000, 300'000, 1'000'000, 3'000'000});
  }

  for (std::size_t n : ns) { //

    std::vector<T> in = random_vec(std::type_identity<T>{}, n);

    // for (auto &&elem : in) {
    //   elem = 1;
    // }

    std::vector<T> const out_ok = check(in);

    // for (auto &&elem : in) {
    //   std::cout << elem << ' ';
    // }
    // std::cout << '\n';

    // for (auto &&elem : out_ok) {
    //   std::cout << elem << ' ';
    // }
    // std::cout << '\n';

    std::vector<int> chunks;

    if (n <= 10) {
      for (int i = 1; i <= n; ++i) {
        chunks.push_back(i);
      }
    } else {
      int ch = 11;
      do {
        chunks.push_back(ch);
        ch *= 11;
      } while (ch <= n);
    }

    for (int chunk : chunks) {

      if (chunk == 0) {
        continue;
      }

      if (chunk > n) {
        break;
      }

      // Test all eight overloads

      std::cout << "n: " << n << " chunk: " << chunk << '\n';

      for (int ll = 0; ll < (n < 10 ? 10 : 1); ++ll) {
        /* [iterator,chunk,output] */ {
          std::vector<T> out(in.size());
          lf::sync_wait(sch, lf::scan, in.begin(), in.end(), out.begin(), chunk, bop, proj);
          REQUIRE(out == out_ok);
        }
        /* [iterator,n = 1,output] */ {
          if (chunk == 1) {
            std::vector<T> out(in.size());
            lf::sync_wait(sch, lf::scan, in.begin(), in.end(), out.begin(), bop, proj);
            REQUIRE(out == out_ok);
          }
        }
        /* [iterator,chunk,in_place] */ {
          std::vector<T> out = in;
          lf::sync_wait(sch, lf::scan, out.begin(), out.end(), chunk, bop, proj);
          REQUIRE(out == out_ok);
        }
        /* [iterator,n = 1,in_place] */ {
          if (chunk == 1) {
            std::vector<T> out = in;
            lf::sync_wait(sch, lf::scan, out.begin(), out.end(), bop, proj);
            REQUIRE(out == out_ok);
          }
        }
        /* [range,chunk,output] */ {
          std::vector<T> out(in.size());
          lf::sync_wait(sch, lf::scan, in, out.begin(), chunk, bop, proj);
          REQUIRE(out == out_ok);
        }
        /* [range,n = 1,output] */ {
          if (chunk == 1) {
            //
            std::vector<T> out(in.size());
            lf::sync_wait(sch, lf::scan, in, out.begin(), bop, proj);
            REQUIRE(out == out_ok);
          }
        }
        /* [range,chunk,in_place] */ {
          std::vector<T> out = in;
          lf::sync_wait(sch, lf::scan, out, chunk, bop, proj);
          REQUIRE(out == out_ok);
        }
        /* [range,n = 1,in_place] */ {
          if (chunk == 1) {
            std::vector<T> out = in;
            lf::sync_wait(sch, lf::scan, out, bop, proj);
            REQUIRE(out == out_ok);
          }
        }
      }
    }
  }
}

template <typename Bop, typename Proj>
auto check(Bop bop, Proj proj) {
  return [bop, proj]<class T>(std::vector<T> const &in) {
    //

    std::vector<T> tmp;

    for (auto &&elem : in) {
      tmp.push_back(proj(elem));
    }

    std::vector<T> out(in.size());

    std::inclusive_scan(tmp.begin(), tmp.end(), out.begin(), bop);

    return out;
  };
}

auto doubler = [](auto const &X) -> decltype(2 * X) {
  return 2 * X;
};

auto coro_doubler = [](auto, auto const &X) -> task<decltype(2 * X)> {
  co_return 2 * X;
};

constexpr auto coro_plus = [](auto, auto const a, auto const b) -> task<decltype(a + b)> {
  co_return a + b;
};

constexpr auto coro_identity = []<typename T>(auto, T &&val) -> task<T &&> {
  co_return std::forward<T>(val);
};

} // namespace

TEMPLATE_TEST_CASE("scan (reg, reg)", "[scan][template]", unit_pool, busy_pool, lazy_pool) {
  SECTION("(+), (id)") {
    test<int>(make_scheduler<TestType>(), std::plus{}, std::identity{}, check(std::plus{}, std::identity{}));
  }
  SECTION("(+), (2*)") {
    test<int>(make_scheduler<TestType>(), std::plus{}, doubler, check(std::plus{}, doubler));
  }
  SECTION("(matmul), (id)") {
    test<matrix>(make_scheduler<TestType>(),
                 std::multiplies{},
                 std::identity{},
                 check(std::multiplies<>{}, std::identity{}));
  }
  SECTION("(string +), (id)") {
    test<std::string>(
        make_scheduler<TestType>(), std::plus{}, std::identity{}, check(std::plus{}, std::identity{}));
  }
}

TEMPLATE_TEST_CASE("scan <int> (co, reg)", "[scan][template]", unit_pool, busy_pool, lazy_pool) {
  SECTION("(+), (id)") {
    test<int>(make_scheduler<TestType>(), coro_plus, std::identity{}, check(std::plus{}, std::identity{}));
  }
  SECTION("(+), (2*)") {
    test<int>(make_scheduler<TestType>(), coro_plus, doubler, check(std::plus{}, doubler));
  }
}

TEMPLATE_TEST_CASE("scan <int> (reg, co)", "[scan][template]", unit_pool, busy_pool, lazy_pool) {
  SECTION("(+), (id)") {
    test<int>(make_scheduler<TestType>(), std::plus{}, coro_identity, check(std::plus{}, std::identity{}));
  }
  SECTION("(+), (2*)") {
    test<int>(make_scheduler<TestType>(), std::plus{}, coro_doubler, check(std::plus{}, doubler));
  }
}

TEMPLATE_TEST_CASE("scan <int> (co, co)", "[scan][template]", unit_pool, busy_pool, lazy_pool) {
  SECTION("(+), (id)") {
    test<int>(make_scheduler<TestType>(), coro_plus, coro_identity, check(std::plus{}, std::identity{}));
  }
  SECTION("(+), (2*)") {
    test<int>(make_scheduler<TestType>(), coro_plus, coro_doubler, check(std::plus{}, doubler));
  }
}
