#include <cstdint>
#include <format>

#include <benchmark/benchmark.h>

#include "fib.hpp"
#include "macros.hpp"

namespace {

auto fib(std::int64_t n) -> std::int64_t {
  if (n < 2) {
    return n;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

#pragma omp task untied shared(lhs) firstprivate(n) default(none)
  lhs = fib(n - 2);

  rhs = fib(n - 1);

#pragma omp taskwait
  return lhs + rhs;
}

template <typename = void>
void fib_run(benchmark::State &state) {
  int threads = static_cast<int>(state.range(1));

  run_fib_mt(state, threads, [threads](std::int64_t n) -> std::int64_t {
    std::int64_t return_value = 0;

#pragma omp parallel num_threads(threads) default(shared)
#pragma omp single nowait
    {
      return_value = fib(n);
    }

    return return_value;
  });
}

} // namespace

BENCH_ALL_MT(fib_run, openmp, fib, fib)
