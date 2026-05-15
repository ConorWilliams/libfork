#include <benchmark/benchmark.h>

#include "heat.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

inline constexpr std::size_t heat_row_chunk = 16;

struct heat_row {
  double const *src;
  double *dst;
  std::size_t n;

  void operator()(std::size_t y) const {
    if (y == 0 || y + 1 == n) {
      for (std::size_t x = 0; x < n; ++x) {
        dst[y * n + x] = src[y * n + x];
      }
      return;
    }

    dst[y * n] = src[y * n];
    for (std::size_t x = 1; x < n - 1; ++x) {
      std::size_t i = y * n + x;
      dst[i] = 0.25 * (src[i - 1] + src[i + 1] + src[i - n] + src[i + n]);
    }
    dst[y * n + (n - 1)] = src[y * n + (n - 1)];
  }
};

struct heat_run {
  template <lf::worker_context Context>
  static auto
  operator()(lf::env<Context>, double *a, double *b, std::size_t n, std::size_t iters)
      -> lf::task<void, Context> {

    double *src = a;
    double *dst = b;

    for (std::size_t t = 0; t < iters; ++t) {
      auto rows = std::views::iota(std::size_t{}, n);
      auto sc = co_await lf::scope();
      co_await sc.call(lf::for_each, rows, heat_row_chunk, heat_row{src, dst, n});
      co_await sc.join();
      std::swap(src, dst);
    }
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);
  lf_bench::report_threads(state, threads);

  run_heat(state, [&](double *a, double *b, std::size_t n, std::size_t iters) {
    lf::schedule(scheduler, heat_run{}, a, b, n, iters).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, heat, heat, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, heat, heat, poly_busy_pool)
