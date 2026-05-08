#include <benchmark/benchmark.h>

#include "fib.hpp"
#include "macros.hpp"

import std;

namespace {

auto fib_impl(std::int64_t &ret, std::int64_t n) -> void {
  if (n < 2) {
    ret = n;
    return;
  }

  std::int64_t lhs = 0;
  std::int64_t rhs = 0;

  fib_impl(lhs, n - 2);
  fib_impl(rhs, n - 1);

  ret = lhs + rhs;
}

template <typename = void>
void fib_serial(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = 0;
    fib_impl(result, n);
    if (result != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", result, expect));
      break;
    }
    benchmark::DoNotOptimize(result);
  }
}

auto fib_ret_impl(std::int64_t n) -> std::int64_t {
  if (n < 2) {
    return n;
  }

  std::int64_t lhs = fib_ret_impl(n - 1);
  std::int64_t rhs = fib_ret_impl(n - 2);

  return lhs + rhs;
}

template <typename = void>
void fib_serial_return(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    std::int64_t result = fib_ret_impl(n);
    if (result != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", result, expect));
      break;
    }
    benchmark::DoNotOptimize(result);
  }
}

constexpr std::size_t m = 10'000;

template <typename = void>
void accumulate(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  std::vector<float> data(m);

  for (std::size_t i = 0; i < m; ++i) {
    data[i] = 2 * i + i % 3;
  }

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    auto result = std::accumulate(data.begin(), data.end(), std::size_t(0));
    benchmark::DoNotOptimize(result);
  }
}

template <typename = void>
void reduce(benchmark::State &state) {

  std::int64_t n = state.range(0);
  std::int64_t expect = fib_ref(n);

  state.counters["n"] = static_cast<double>(n);

  std::vector<float> data(m);

  for (std::size_t i = 0; i < m; ++i) {
    data[i] = 2 * i + i % 3;
  }

  for (auto _ : state) {
    benchmark::DoNotOptimize(n);
    auto result = std::reduce(data.begin(), data.end(), std::size_t(0));
    benchmark::DoNotOptimize(result);
  }
}

} // namespace

BENCH_ALL(reduce, serial, add_red, fib)
BENCH_ALL(accumulate, serial, add_acc, fib)

BENCH_ALL(fib_serial, serial, fib, fib)
BENCH_ALL(fib_serial_return, serial, fib / return, fib)
