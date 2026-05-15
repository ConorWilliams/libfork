#include <benchmark/benchmark.h>

#include "mandelbrot.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

inline constexpr int mandelbrot_row_chunk = 8;

struct mandelbrot_row {
  int n;

  auto operator()(int py) const -> std::uint64_t {
    std::uint64_t checksum = 0;
    for (int px = 0; px < n; ++px) {
      checksum += static_cast<std::uint64_t>(mandelbrot_pixel(px, py, n));
    }
    return checksum;
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);
  lf_bench::report_threads(state, threads);

  run_mandelbrot(state, [&](int n) -> std::uint64_t {
    auto rows = std::views::iota(0, n);
    return *lf::schedule(scheduler, lf::fold, rows, mandelbrot_row_chunk, std::plus<>{}, mandelbrot_row{n}).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, mandelbrot, mandelbrot, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, mandelbrot, mandelbrot, poly_busy_pool)
