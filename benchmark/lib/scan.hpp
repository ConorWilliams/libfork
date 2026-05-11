#pragma once

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <vector>
#else
import std;
#endif

inline constexpr std::size_t scan_test = 1'000;
inline constexpr std::size_t scan_base = 8'000;

inline constexpr std::size_t scan_reps = 1'000;

inline auto scan_make_vec(std::size_t n) -> std::vector<unsigned> {
  std::vector<unsigned> out(n);
  unsigned count = 0;
  for (auto &elem : out) {
    elem = ++count;
  }
  return out;
}

template <typename Fn>
void run_scan(benchmark::State &state, Fn fn) {
  auto n = static_cast<std::size_t>(state.range(0));
  state.counters["n"] = static_cast<double>(n);
  state.counters["reps"] = static_cast<double>(scan_reps);

  std::vector<unsigned> in = scan_make_vec(n);
  std::vector<unsigned> out(n);

  // For 1..n the inclusive scan's last element equals n*(n+1)/2 (mod 2^32).
  unsigned expect = static_cast<unsigned>(static_cast<std::uint64_t>(n) * (n + 1) / 2);

  lf_bench::bench(state, expect, [&]() -> unsigned {
    std::invoke(fn, in, out, scan_reps);
    benchmark::DoNotOptimize(out.data());
    return out.back();
  });
}
