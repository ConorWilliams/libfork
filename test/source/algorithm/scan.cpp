
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <type_traits>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "matrix.hpp"

#include "libfork/core.hpp"
#include "libfork/schedule.hpp"

#include "libfork/algorithm/scan.hpp"

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
  std::uniform_int_distribution<char> dist{'a', 'z'};

  for (auto &&elem : out) {
    elem.push_back(dist(rng));
  }

  return out;
}

template <typename T, typename Sch, typename Check, typename F, typename Proj = std::identity>
void test(Sch &&sch, F bop, Proj proj, Check check) {

  constexpr std::size_t max_elems = 50;

  for (std::size_t n = 5; n < max_elems; n++) { //

    std::vector<T> const in = random_vec(std::type_identity<T>{}, n);
    std::vector<T> const out_ok = check(in);

    // for (auto &&elem : in) {
    //   std::cout << elem << ' ';
    // }
    // std::cout << '\n';

    // for (auto &&elem : out_ok) {
    //   std::cout << elem << ' ';
    // }
    // std::cout << '\n';

    for (int chunk = 1; chunk <= static_cast<int>(max_elems) + 1; chunk++) {

      // Test all eight overloads

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
