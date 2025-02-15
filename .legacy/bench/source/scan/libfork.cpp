#include <iostream>
#include <ranges>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "../util.hpp"
#include "config.hpp"

using namespace lf;

namespace {

constexpr auto repeat = [](auto, unsigned const * in, unsigned * ou) -> lf::task<void> {
  for (std::size_t i = 0; i < scan_reps; ++i) {
    // std::inclusive_scan(in, in + scan_n, ou, std::plus<>{}); ///
    co_await lf::just(lf::scan)(in, in + scan_n, ou, scan_chunk, std::plus<>{}); 
  }
  co_return;
};

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void scan_libfork(benchmark::State &state) {

  state.counters["green_threads"] = static_cast<double>(state.range(0));
  state.counters["n"] = scan_n;
  state.counters["reps"] = scan_reps;
  state.counters["chunk"] = scan_chunk;

  Sch sch = [&] {
    if constexpr (std::constructible_from<Sch, int>) {
      return Sch(state.range(0));
    } else {
      return Sch{};
    }
  }();

  std::vector in = lf::sync_wait(sch, lf::lift, make_vec);

  std::vector ou = lf::sync_wait(sch, lf::lift, [&] {
    return std::vector{in};
  });

  volatile unsigned sink = 0;

  for (auto _ : state) {
    lf::sync_wait(sch, repeat, in.data(), ou.data());
  }

  sink = ou.back();
}

} // namespace

// BENCHMARK(scan_libfork<lazy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
BENCHMARK(scan_libfork<lazy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();

// BENCHMARK(scan_libfork<busy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
// BENCHMARK(scan_libfork<busy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();