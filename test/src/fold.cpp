#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

namespace {

using mono_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using poly_pool = lf::poly_busy_pool<lf::geometric_stack<>>;

constexpr std::array<std::size_t, 3> k_worker_counts{1, 2, 4};
constexpr std::array<std::size_t, 11> k_sizes{1, 2, 3, 4, 5, 6, 8, 9, 97, 1024, 4096};

constexpr auto sum_0_to_n_minus_1(std::size_t n) -> std::size_t { return (n * (n - 1)) / 2; }

constexpr auto sum_squares_0_to_n_minus_1(std::size_t n) -> std::size_t {
  return n == 0 ? 0 : (n * (n - 1) * (2 * n - 1)) / 6;
}

} // namespace

TEMPLATE_TEST_CASE("fold: iterator-pair, n=1 (no n parameter)", "[fold]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {

    TestType pool{n_workers};

    for (auto n : k_sizes) {
      DYNAMIC_SECTION("workers=" << n_workers << " len=" << n) {

        std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};

        auto recv = lf::schedule(pool, lf::fold, v.begin(), v.end(), std::plus<>{});

        REQUIRE(std::move(recv).get() == sum_0_to_n_minus_1(n));
      }
    }
  }
}

TEMPLATE_TEST_CASE("fold: iterator-pair, n>1", "[fold]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {

    TestType pool{n_workers};

    for (auto n : k_sizes) {
      for (auto ch : k_sizes) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {

          std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};

          auto recv = lf::schedule(pool, lf::fold, v.begin(), v.end(), ch, std::plus<>{});

          REQUIRE(std::move(recv).get() == sum_0_to_n_minus_1(n));
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("fold: range + n", "[fold]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {

    TestType pool{n_workers};

    for (auto n : k_sizes) {
      for (auto ch : k_sizes) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {

          std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};

          auto recv = lf::schedule(pool, lf::fold, std::span(v), ch, std::plus<>{});

          REQUIRE(std::move(recv).get() == sum_0_to_n_minus_1(n));
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("fold: range no n (default chunk)", "[fold]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {

    TestType pool{n_workers};

    for (auto n : k_sizes) {
      DYNAMIC_SECTION("workers=" << n_workers << " len=" << n) {

        std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};

        auto recv = lf::schedule(pool, lf::fold, std::span(v), std::plus<>{});

        REQUIRE(std::move(recv).get() == sum_0_to_n_minus_1(n));
      }
    }
  }
}

TEMPLATE_TEST_CASE("fold: non-trivial sync projection (sum of squares)", "[fold]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {

    TestType pool{n_workers};

    for (auto n : k_sizes) {
      for (auto ch : k_sizes) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {

          std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};

          auto recv =
              lf::schedule(pool, lf::fold, std::span(v), ch, std::plus<>{}, [](std::size_t x) -> std::size_t {
                return x * x;
              });

          REQUIRE(std::move(recv).get() == sum_squares_0_to_n_minus_1(n));
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("fold: stateful Bop (counter increment)", "[fold]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {

    TestType pool{n_workers};

    for (auto n : k_sizes) {
      DYNAMIC_SECTION("workers=" << n_workers << " len=" << n) {

        std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};

        struct counting_plus {
          std::atomic<std::size_t> *calls;
          auto operator()(std::size_t a, std::size_t b) const -> std::size_t {
            calls->fetch_add(1, std::memory_order_relaxed);
            return a + b;
          }
        };

        std::atomic<std::size_t> calls{0};

        auto recv = lf::schedule(pool, lf::fold, std::span(v), counting_plus{&calls});

        REQUIRE(std::move(recv).get() == sum_0_to_n_minus_1(n));
        REQUIRE(calls.load() == n - 1UZ);
      }
    }
  }
}

namespace {

struct async_plus {
  template <typename Context>
  static auto operator()(lf::env<Context>, std::size_t a, std::size_t b) -> lf::task<std::size_t, Context> {
    co_return a + b;
  }
};

struct async_square {
  template <typename Context>
  static auto operator()(lf::env<Context>, std::size_t x) -> lf::task<std::size_t, Context> {
    co_return x *x;
  }
};

} // namespace

TEMPLATE_TEST_CASE("fold: async Bop — iterator-pair, n=1", "[fold]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    TestType pool{n_workers};
    for (auto n : k_sizes) {
      DYNAMIC_SECTION("workers=" << n_workers << " len=" << n) {
        std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};
        auto recv = lf::schedule(pool, lf::fold, v.begin(), v.end(), async_plus{});
        REQUIRE(std::move(recv).get() == sum_0_to_n_minus_1(n));
      }
    }
  }
}

TEMPLATE_TEST_CASE("fold: async Bop — range + n", "[fold]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    TestType pool{n_workers};
    for (auto n : k_sizes) {
      for (auto ch : k_sizes) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {
          std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};
          auto recv = lf::schedule(pool, lf::fold, std::span(v), ch, async_plus{});
          REQUIRE(std::move(recv).get() == sum_0_to_n_minus_1(n));
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("fold: async Proj — iterator-pair, n>1", "[fold]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    TestType pool{n_workers};
    for (auto n : k_sizes) {
      for (auto ch : k_sizes) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {
          std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};
          auto recv = lf::schedule(pool, lf::fold, v.begin(), v.end(), ch, std::plus<>{}, async_square{});
          REQUIRE(std::move(recv).get() == sum_squares_0_to_n_minus_1(n));
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("fold: async Bop + async Proj — range + n", "[fold]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    TestType pool{n_workers};
    for (auto n : k_sizes) {
      for (auto ch : k_sizes) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {
          std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};
          auto recv = lf::schedule(pool, lf::fold, std::span(v), ch, async_plus{}, async_square{});
          REQUIRE(std::move(recv).get() == sum_squares_0_to_n_minus_1(n));
        }
      }
    }
  }
}

#if LF_COMPILER_EXCEPTIONS

TEMPLATE_TEST_CASE("fold: exception from Bop propagates", "[fold]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n_workers) {

      TestType pool{n_workers};

      std::vector v{std::from_range, std::ranges::iota_view(0UZ, 1024UZ)};

      auto recv = lf::schedule(
          pool, lf::fold, v.begin(), v.end(), 4UZ, [](std::size_t a, std::size_t b) -> std::size_t {
            if (a == 500 || b == 500) {
              throw std::runtime_error{"boom"};
            }
            return a + b;
          });

      REQUIRE_THROWS_AS(std::move(recv).get(), std::runtime_error);
    }
  }
}

#endif // LF_COMPILER_EXCEPTIONS
