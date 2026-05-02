#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

import std;

import libfork;

namespace {

using mono_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using poly_pool = lf::poly_busy_pool<lf::geometric_stack<>>;

constexpr std::array<std::size_t, 3> k_worker_counts{1, 2, 4};

} // namespace

TEMPLATE_TEST_CASE("for_each: iterator-pair, n=1 (no n parameter)", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n_workers) {
      TestType pool{n_workers};

      std::vector<int> v(1024);
      std::iota(v.begin(), v.end(), 0);

      auto recv = lf::schedule(pool, lf::for_each, v.begin(), v.end(), [](int &x) noexcept {
        x *= 2;
      });
      std::move(recv).get();

      for (std::size_t i = 0; i < v.size(); ++i) {
        REQUIRE(v[i] == 2 * static_cast<int>(i));
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: iterator-pair, n>1", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n_workers) {
      TestType pool{n_workers};

      std::vector<int> v(4096);
      std::iota(v.begin(), v.end(), 0);

      auto recv =
          lf::schedule(pool, lf::for_each, v.begin(), v.end(), std::ptrdiff_t{64}, [](int &x) noexcept {
            x += 1;
          });
      std::move(recv).get();

      for (std::size_t i = 0; i < v.size(); ++i) {
        REQUIRE(v[i] == static_cast<int>(i) + 1);
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: range + n=1 dispatch", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n_workers) {
      TestType pool{n_workers};

      std::vector<int> v(512);
      std::iota(v.begin(), v.end(), 1);

      auto recv = lf::schedule(pool, lf::for_each, std::span<int>(v), std::ptrdiff_t{1}, [](int &x) noexcept {
        x = -x;
      });
      std::move(recv).get();

      for (std::size_t i = 0; i < v.size(); ++i) {
        REQUIRE(v[i] == -(static_cast<int>(i) + 1));
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: range + n>1 dispatch", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n_workers) {
      TestType pool{n_workers};

      std::vector<int> v(10000, 7);

      auto recv =
          lf::schedule(pool, lf::for_each, std::span<int>(v), std::ptrdiff_t{10}, [](int &x) noexcept {
            x = x * x;
          });
      std::move(recv).get();

      for (auto x : v) {
        REQUIRE(x == 49);
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: range no n (default chunk)", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n_workers) {
      TestType pool{n_workers};

      std::vector<int> v(256, 0);

      auto recv = lf::schedule(pool, lf::for_each, std::span<int>(v), [](int &x) noexcept {
        x = 42;
      });
      std::move(recv).get();

      for (auto x : v) {
        REQUIRE(x == 42);
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: empty range", "[for_each]", mono_pool, poly_pool) {
  TestType pool{2};

  std::vector<int> v;
  std::atomic<int> calls{0};

  SECTION("iterator pair, n=1") {
    auto recv = lf::schedule(pool, lf::for_each, v.begin(), v.end(), [&](int &) noexcept {
      calls.fetch_add(1, std::memory_order_relaxed);
    });
    std::move(recv).get();
    REQUIRE(calls.load() == 0);
  }

  SECTION("iterator pair, n>1") {
    auto recv = lf::schedule(pool, lf::for_each, v.begin(), v.end(), std::ptrdiff_t{8}, [&](int &) noexcept {
      calls.fetch_add(1, std::memory_order_relaxed);
    });
    std::move(recv).get();
    REQUIRE(calls.load() == 0);
  }
}

TEMPLATE_TEST_CASE("for_each: single element range", "[for_each]", mono_pool, poly_pool) {
  TestType pool{2};

  std::vector<int> v{99};

  auto recv = lf::schedule(pool, lf::for_each, v.begin(), v.end(), [](int &x) noexcept {
    x = -x;
  });
  std::move(recv).get();

  REQUIRE(v[0] == -99);
}

TEMPLATE_TEST_CASE("for_each: with projection", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n_workers) {
      TestType pool{n_workers};

      // Pair: write the result of (.first squared) into .second via projection
      std::vector<std::pair<int, int>> v;
      v.reserve(1024);
      for (int i = 0; i < 1024; ++i) {
        v.emplace_back(i, 0);
      }

      auto recv = lf::schedule(
          pool,
          lf::for_each,
          v.begin(),
          v.end(),
          std::ptrdiff_t{32},
          [](std::pair<int, int> &p) noexcept {
            p.second = p.first * p.first;
          },
          [](std::pair<int, int> &p) noexcept -> std::pair<int, int> & {
            return p;
          } //
      );
      std::move(recv).get();

      for (int i = 0; i < 1024; ++i) {
        REQUIRE(v[static_cast<std::size_t>(i)].first == i);
        REQUIRE(v[static_cast<std::size_t>(i)].second == i * i);
      }
    }
  }
}
