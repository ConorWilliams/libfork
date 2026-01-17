#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <numeric>

#include <ranges>

#include <benchmark/benchmark.h>

#include <tbb/blocked_range.h>
#include <tbb/global_control.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

void primes_tbb(benchmark::State &state) {

  // TBB uses (2MB) stacks by default
  tbb::global_control global_limit(tbb::global_control::thread_stack_size, 8 * 1024 * 1024);

  state.counters["green_threads"] = static_cast<double>(state.range(0));
  state.counters["primes(n)"] = primes_lim;
  state.counters["primes_chunk"] = primes_chunk;

  std::size_t n = state.range(0);
  tbb::task_arena arena(n);

  volatile int secret = primes_lim;
  volatile int output = 0;

  for (auto _ : state) {
    output = arena.execute([&] {
      return tbb::parallel_reduce(
          tbb::blocked_range(1, secret, primes_chunk),
          0,
          [&](auto range, auto sum) {
            for (auto i = range.begin(); i < range.end(); ++i) {
              sum += is_prime(i);
            }
            return sum;
          },
          std::plus<>());
    });
  }
}

} // namespace

BENCHMARK(primes_tbb)->Apply(targs)->UseRealTime();
