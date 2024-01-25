#include <iostream>
#include <ranges>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "../util.hpp"
#include "config.hpp"

using namespace lf;

namespace {

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void scan_libfork(benchmark::State &state) {

  state.counters["green_threads"] = static_cast<double>(state.range(0));
  state.counters["scan(n)"] = scan_n;
  state.counters["scan_chunk"] = scan_chunk;

  Sch sch = [&] {
    if constexpr (std::constructible_from<Sch, int>) {
      return Sch(state.range(0));
    } else {
      return Sch{};
    }
  }();

  std::vector<unsigned> in = lf::sync_wait(sch, lf::lift, make_vec);
  std::vector<unsigned> ou(in.size());
  volatile unsigned sink = 0;

  for (auto _ : state) {
    
    lf::sync_wait(sch, lf::scan, in, ou.begin(), scan_chunk, std::plus<>{});

    sink = ou.back();
  }
}

} // namespace

// BENCHMARK(scan_libfork<lazy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
BENCHMARK(scan_libfork<lazy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();

// BENCHMARK(scan_libfork<busy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
// BENCHMARK(scan_libfork<busy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();