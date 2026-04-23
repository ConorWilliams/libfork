#include <benchmark/benchmark.h>
#include <omp.h>

#include "common.hpp"
#include "fib.hpp"
#include "helpers.hpp"

import std;

namespace {

auto fib_omp_impl(std::int64_t n) -> std::int64_t {
  if (n < 2) {
    return n;
  }

  // Use a cutoff to avoid creating too many tasks
  if (n < 20) {
    auto fib_serial = [](auto self, std::int64_t val) -> std::int64_t {
      if (val < 2) return val;
      return self(self, val - 1) + self(self, val - 2);
    };
    return fib_serial(fib_serial, n);
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  #pragma omp task shared(lhs)
  lhs = fib_omp_impl(n - 1);

  #pragma omp task shared(rhs)
  rhs = fib_omp_impl(n - 2);

  #pragma omp taskwait
  return lhs + rhs;
}

template <typename = void>
void run(benchmark::State &state) {
  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);
  int threads = static_cast<int>(state.range(1));

  state.counters["n"] = static_cast<double>(n);
  state.counters["p"] = static_cast<double>(threads);
  state.SetComplexityN(static_cast<benchmark::IterationCount>(threads));

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t return_value = 0;

    omp_set_num_threads(threads);
    #pragma omp parallel
    {
      #pragma omp single
      {
        return_value = fib_omp_impl(n);
      }
    }

    if (return_value != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", return_value, expect));
      break;
    }

    benchmark::DoNotOptimize(return_value);
  }
}

} // namespace

OMP_BENCH_ALL(run, fib)
