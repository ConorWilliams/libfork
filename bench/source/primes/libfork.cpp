#include <iostream>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "../util.hpp"
#include "config.hpp"

using namespace lf;

namespace {

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void primes_libfork(benchmark::State &state) {

  state.counters["green_threads"] = state.range(0);
  state.counters["primes(n)"] = primes_lim;

  Sch sch = [&] {
    if constexpr (std::constructible_from<Sch, int>) {
      return Sch(state.range(0));
    } else {
      return Sch{};
    }
  }();

  volatile int secret = primes_lim;
  volatile int output = 0;

  for (auto _ : state) {
    output = *lf::sync_wait(sch, lf::fold, std::ranges::views::iota(1, secret), 10, std::plus<>{}, is_prime);
  }
}

} // namespace

BENCHMARK(primes_libfork<lazy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
BENCHMARK(primes_libfork<lazy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();

BENCHMARK(primes_libfork<busy_pool, numa_strategy::seq>)->Apply(targs)->UseRealTime();
BENCHMARK(primes_libfork<busy_pool, numa_strategy::fan>)->Apply(targs)->UseRealTime();