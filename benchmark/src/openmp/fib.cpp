#include <benchmark/benchmark.h>

#include "common.hpp"
#include "fib.hpp"
#include "macros.hpp"

#include <cstdint>
#include <format>

namespace {

auto fib_omp_impl(std::int64_t n) -> std::int64_t {
  if (n < 2) {
    return n;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

#pragma omp task untied shared(lhs) firstprivate(n) default(none)
  lhs = fib_omp_impl(n - 1);

  rhs = fib_omp_impl(n - 2);

#pragma omp taskwait
  return lhs + rhs;
}

template <typename = void>
void fib_run(benchmark::State &state) {
  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);
  int threads = static_cast<int>(state.range(1));

  state.counters["n"] = static_cast<double>(n);
  state.counters["p"] = static_cast<double>(threads);
  state.SetComplexityN(static_cast<benchmark::IterationCount>(threads));

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t return_value = 0;

#pragma omp parallel num_threads(threads) default(shared)
#pragma omp single nowait
    {
      return_value = fib_omp_impl(n);
    }

    if (return_value != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", return_value, expect));
      break;
    }

    benchmark::DoNotOptimize(return_value);
  }
}

} // namespace

BENCH_ALL_MT(fib_run, openmp, fib, fib)
