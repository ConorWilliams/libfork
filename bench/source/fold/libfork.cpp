#include <iostream>
#include <ranges>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "../util.hpp"
#include "config.hpp"

using namespace lf;

namespace {

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void fold_libfork(benchmark::State &state) {

  state.counters["green_threads"] = static_cast<double>(state.range(0));
  state.counters["fold(n)"] = fold_n;
  state.counters["fold_chunk"] = fold_chunk;

  Sch sch = [&] {
    if constexpr (std::constructible_from<Sch, int>) {
      return Sch(state.range(0));
    } else {
      return Sch{};
    }
  }();

  std::vector<unsigned> in = lf::sync_wait(sch, lf::lift, make_vec_fold);
  volatile unsigned sink = 0;



  for (auto _ : state) {
    sink = *lf::sync_wait(sch, lf::fold, std::ranges::views::iota(fold_n), fold_chunk, std::plus<>{});
  }

  // std::cout << sink << std::endl;
}

} // namespace

// BENCHMARK(fold_libfork<lazy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
BENCHMARK(fold_libfork<lazy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();

// BENCHMARK(fold_libfork<busy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
// BENCHMARK(fold_libfork<busy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();