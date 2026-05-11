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
  run_fib(state, [](std::int64_t n) -> std::int64_t {
    std::int64_t result = 0;
    fib_impl(result, n);
    return result;
  });
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
  run_fib(state, fib_ret_impl);
}

} // namespace

BENCH_ALL(fib_serial, serial, fib, fib)
BENCH_ALL(fib_serial_return, serial, fib / return, fib)
