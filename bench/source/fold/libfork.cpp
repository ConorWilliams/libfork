#include <iostream>
#include <ranges>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "../util.hpp"
#include "config.hpp"

using namespace lf;

namespace {

constexpr auto repeat_nest = [](auto, std::vector<unsigned> const &in) -> lf::task<unsigned> {
  co_return *co_await lf::just(lf::fold)(
      std::ranges::views::iota(0UL, fold_reps), std::plus<>{}, [&](auto, int) -> task<unsigned> {
        co_return *co_await lf::just(lf::fold)(in, fold_chunk, std::plus<>{});
      });
};

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
    if constexpr (Nest) {
      sink = lf::sync_wait(sch, repeat_nest, in);
    } else {
      sink = lf::sync_wait(sch, repeat, in);
    }
  }
}

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void nest_fold_libfork(benchmark::State &state) {
  fold_libfork<Sch, Strategy, true>(state);
}

} // namespace

// BENCHMARK(nest_fold_libfork<lazy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
BENCHMARK(nest_fold_libfork<lazy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();

// BENCHMARK(nest_fold_libfork<busy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
// BENCHMARK(nest_fold_libfork<busy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();

// BENCHMARK(fold_libfork<lazy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
BENCHMARK(fold_libfork<lazy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();

// BENCHMARK(fold_libfork<busy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
// BENCHMARK(fold_libfork<busy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();