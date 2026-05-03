#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

import std;

import libfork;

namespace {

using mono_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using poly_pool = lf::poly_busy_pool<lf::geometric_stack<>>;

constexpr std::array<std::size_t, 3> k_worker_counts{1, 2, 4};
constexpr std::array<std::size_t, 12> k_sizes{0, 1, 2, 3, 4, 5, 6, 8, 9, 97, 1024, 4096};

} // namespace

TEMPLATE_TEST_CASE("for_each: iterator-pair, n=1 (no n parameter)", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    for (auto n : k_sizes) {
      DYNAMIC_SECTION("workers=" << n_workers << " len=" << n) {
        TestType pool{n_workers};

        std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};

        auto recv = lf::schedule(pool, lf::for_each, v.begin(), v.end(), [](std::size_t &x) -> void {
          x += 1;
        });

        std::move(recv).get();

        for (std::size_t i = 0; i < v.size(); ++i) {
          REQUIRE(v[i] == i + 1);
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: iterator-pair, n>1", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    for (auto n : k_sizes) {
      for (auto ch : std::span(k_sizes).subspan(1)) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {
          TestType pool{n_workers};

          std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};

          auto recv = lf::schedule(pool, lf::for_each, v.begin(), v.end(), ch, [](std::size_t &x) -> void {
            x += 1;
          });

          std::move(recv).get();

          for (std::size_t i = 0; i < v.size(); ++i) {
            REQUIRE(v[i] == i + 1);
          }
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: range + n", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    for (auto n : k_sizes) {
      for (auto ch : std::span(k_sizes).subspan(1)) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {
          TestType pool{n_workers};

          std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};

          auto recv = lf::schedule(pool, lf::for_each, std::span(v), ch, [](std::size_t &x) -> void {
            x += 1;
          });

          std::move(recv).get();

          for (std::size_t i = 0; i < v.size(); ++i) {
            REQUIRE(v[i] == i + 1);
          }
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: range no n (default chunk)", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    for (auto n : k_sizes) {
      DYNAMIC_SECTION("workers=" << n_workers << " len=" << n) {
        TestType pool{n_workers};

        std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};

        auto recv = lf::schedule(pool, lf::for_each, std::span(v), [](std::size_t &x) -> void {
          x += 1;
        });

        std::move(recv).get();

        for (std::size_t i = 0; i < v.size(); ++i) {
          REQUIRE(v[i] == i + 1);
        }
      }
    }
  }
}
