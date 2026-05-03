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

    TestType pool{n_workers};

    for (auto n : k_sizes) {
      DYNAMIC_SECTION("workers=" << n_workers << " len=" << n) {

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

    TestType pool{n_workers};

    for (auto n : k_sizes) {
      for (auto ch : std::span(k_sizes).subspan(1)) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {

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

    TestType pool{n_workers};

    for (auto n : k_sizes) {
      for (auto ch : std::span(k_sizes).subspan(1)) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {

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

    TestType pool{n_workers};

    for (auto n : k_sizes) {
      DYNAMIC_SECTION("workers=" << n_workers << " len=" << n) {

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

TEMPLATE_TEST_CASE("for_each: exact invocation count and read-only Fn", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {

    TestType pool{n_workers};

    for (auto n : k_sizes) {
      for (auto ch : std::span(k_sizes).subspan(1)) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {

          std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};

          std::atomic<std::size_t> count{0};
          std::atomic<std::size_t> sum{0};

          auto recv = lf::schedule(
              pool, lf::for_each, std::span(v), ch, [&count, &sum](std::size_t const &x) -> void {
                count.fetch_add(1, std::memory_order_relaxed);
                sum.fetch_add(x, std::memory_order_relaxed);
              });

          std::move(recv).get();

          REQUIRE(count.load() == n);
          REQUIRE(sum.load() == (n * (n - 1)) / 2);

          for (std::size_t i = 0; i < v.size(); ++i) {
            REQUIRE(v[i] == i);
          }
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: stateful Fn (captured-by-ref counter)", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {

    TestType pool{n_workers};

    for (auto n : k_sizes) {
      DYNAMIC_SECTION("workers=" << n_workers << " len=" << n) {

        std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};

        struct stateful {
          std::atomic<std::size_t> *hits;
          std::size_t tag;
          void operator()(std::size_t &x) const {
            hits->fetch_add(1, std::memory_order_relaxed);
            x += tag;
          }
        };

        std::atomic<std::size_t> hits{0};

        auto recv = lf::schedule(pool, lf::for_each, std::span(v), stateful{&hits, 7});

        std::move(recv).get();

        REQUIRE(hits.load() == n);
        for (std::size_t i = 0; i < v.size(); ++i) {
          REQUIRE(v[i] == i + 7);
        }
      }
    }
  }
}

namespace {

struct async_inc {
  template <typename Context>
  static auto operator()(lf::env<Context>, std::size_t &x) -> lf::task<void, Context> {
    x += 1;
    co_return;
  }
};

struct async_count_sum {
  std::atomic<std::size_t> *count;
  std::atomic<std::size_t> *sum;

  template <typename Context>
  auto operator()(lf::env<Context>, std::size_t const &x) const -> lf::task<void, Context> {
    count->fetch_add(1, std::memory_order_relaxed);
    sum->fetch_add(x, std::memory_order_relaxed);
    co_return;
  }
};

} // namespace

TEMPLATE_TEST_CASE("for_each: async Fn — iterator-pair, n=1", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    TestType pool{n_workers};
    for (auto n : k_sizes) {
      DYNAMIC_SECTION("workers=" << n_workers << " len=" << n) {
        std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};
        auto recv = lf::schedule(pool, lf::for_each, v.begin(), v.end(), async_inc{});
        std::move(recv).get();
        for (std::size_t i = 0; i < v.size(); ++i) {
          REQUIRE(v[i] == i + 1);
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: async Fn — iterator-pair, n>1", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    TestType pool{n_workers};
    for (auto n : k_sizes) {
      for (auto ch : std::span(k_sizes).subspan(1)) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {
          std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};
          auto recv = lf::schedule(pool, lf::for_each, v.begin(), v.end(), ch, async_inc{});
          std::move(recv).get();
          for (std::size_t i = 0; i < v.size(); ++i) {
            REQUIRE(v[i] == i + 1);
          }
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: async Fn — range + n", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    TestType pool{n_workers};
    for (auto n : k_sizes) {
      for (auto ch : std::span(k_sizes).subspan(1)) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {
          std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};
          auto recv = lf::schedule(pool, lf::for_each, std::span(v), ch, async_inc{});
          std::move(recv).get();
          for (std::size_t i = 0; i < v.size(); ++i) {
            REQUIRE(v[i] == i + 1);
          }
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: async Fn — range no n", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    TestType pool{n_workers};
    for (auto n : k_sizes) {
      DYNAMIC_SECTION("workers=" << n_workers << " len=" << n) {
        std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};
        auto recv = lf::schedule(pool, lf::for_each, std::span(v), async_inc{});
        std::move(recv).get();
        for (std::size_t i = 0; i < v.size(); ++i) {
          REQUIRE(v[i] == i + 1);
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: async Fn — exact invocation count", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    TestType pool{n_workers};
    for (auto n : k_sizes) {
      for (auto ch : std::span(k_sizes).subspan(1)) {
        DYNAMIC_SECTION("workers=" << n_workers << " len=" << n << " chunk=" << ch) {
          std::vector v{std::from_range, std::ranges::iota_view(0UZ, n)};
          std::atomic<std::size_t> count{0};
          std::atomic<std::size_t> sum{0};
          auto recv =
              lf::schedule(pool, lf::for_each, std::span(v), ch, async_count_sum{&count, &sum});
          std::move(recv).get();
          REQUIRE(count.load() == n);
          REQUIRE(sum.load() == (n * (n - 1)) / 2);
          for (std::size_t i = 0; i < v.size(); ++i) {
            REQUIRE(v[i] == i);
          }
        }
      }
    }
  }
}

TEMPLATE_TEST_CASE("for_each: exception from Fn propagates", "[for_each]", mono_pool, poly_pool) {
  for (auto n_workers : k_worker_counts) {
    DYNAMIC_SECTION("workers=" << n_workers) {

      TestType pool{n_workers};

      std::vector v{std::from_range, std::ranges::iota_view(0UZ, 1024UZ)};

      auto recv = lf::schedule(pool, lf::for_each, v.begin(), v.end(), [](std::size_t &x) -> void {
        if (x == 500) {
          throw std::runtime_error{"boom"};
        }
      });

      REQUIRE_THROWS_AS(std::move(recv).get(), std::runtime_error);
    }
  }
}
