#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "scan.hpp"

import std;

namespace {

template <typename = void>
void scan_serial(benchmark::State &state) {

  std::size_t n = static_cast<std::size_t>(state.range(0));
  state.counters["n"] = static_cast<double>(n);
  state.counters["reps"] = scan_reps;

  std::vector<unsigned> in = scan_make_vec(n);
  std::vector<unsigned> out(n);

  // For 1..n the inclusive scan's last element equals n*(n+1)/2 (mod 2^32).
  unsigned expect = static_cast<unsigned>(static_cast<std::uint64_t>(n) * (n + 1) / 2);

  for (auto _ : state) {
    for (std::size_t i = 0; i < scan_reps; ++i) {
      std::inclusive_scan(in.begin(), in.end(), out.begin(), std::plus<>{});
    }
    if (out.back() != expect) {
      state.SkipWithError(std::format("incorrect scan tail: {} != {}", out.back(), expect));
      break;
    }
    benchmark::DoNotOptimize(out.data());
  }
}

} // namespace

BENCH_ALL(scan_serial, serial, scan, scan)
