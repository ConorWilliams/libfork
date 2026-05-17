#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "scan.hpp"

import std;

namespace {

template <typename = void>
void scan_serial(benchmark::State &state) {
  run_scan(state, [](std::vector<unsigned> const &in, std::vector<unsigned> &out, std::size_t reps) {
    for (std::size_t i = 0; i < reps; ++i) {
      std::inclusive_scan(in.begin(), in.end(), out.begin(), std::plus<>{});
    }
  });
}

} // namespace

BENCH_ALL(scan_serial, serial, scan, scan)
