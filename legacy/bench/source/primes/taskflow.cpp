#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <numeric>
#include <random>
#include <ranges>

#include <benchmark/benchmark.h>

#include <taskflow/algorithm/partitioner.hpp>
#include <taskflow/algorithm/reduce.hpp>
#include <taskflow/taskflow.hpp>

#include "../util.hpp"
#include "config.hpp"

namespace {

void primes_ztaskflow(benchmark::State &state) {

  state.counters["green_threads"] = static_cast<double>(state.range(0));
  state.counters["primes(n)"] = primes_lim;
  state.counters["primes_chunk"] = primes_chunk;

  std::size_t n = state.range(0);
  tf::Executor executor(n);

  volatile int secret = primes_lim;
  volatile int output = 0;

  for (auto _ : state) {

    tf::Taskflow taskflow;

    auto iota = std::ranges::views::iota(1, secret);
    auto rev = std::ranges::views::reverse(iota);

    int sum = 0;

    tf::Task task = taskflow.transform_reduce(
        rev.begin(), rev.end(), sum, std::plus<>{}, ::is_prime, tf::DynamicPartitioner(primes_chunk) //
    );

    executor.run(taskflow).wait();

    output = sum;
  }
}

} // namespace

BENCHMARK(primes_ztaskflow)->Apply(targs)->UseRealTime();
