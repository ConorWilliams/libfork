#include <iostream>
#include <ranges>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "../util.hpp"
#include "config.hpp"

using namespace lf;

namespace {

constexpr auto repeat = [](auto, std::vector<unsigned> const &in) -> lf::task<unsigned> {
  unsigned sum = 0;

  for (std::size_t i = 0; i < fold_reps; ++i) {
    sum += *co_await lf::just(lf::fold)(in, fold_chunk, std::plus<>{});
  }

  co_return sum;
};

template <lf::scheduler Sch, lf::numa_strategy Strategy, bool Nest = false>
void fold_libfork(benchmark::State &state) {

  state.counters["green_threads"] = static_cast<double>(state.range(0));
  state.counters["n"] = fold_n;
  state.counters["reps"] = fold_reps;
  state.counters["chunk"] = fold_chunk;

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
    sink = lf::sync_wait(sch, repeat, in);
  }
}

} // namespace

// BENCHMARK(nest_fold_libfork<busy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
// BENCHMARK(nest_fold_libfork<busy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();

// BENCHMARK(fold_libfork<lazy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
BENCHMARK(fold_libfork<lazy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();

// BENCHMARK(fold_libfork<busy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
// BENCHMARK(fold_libfork<busy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();