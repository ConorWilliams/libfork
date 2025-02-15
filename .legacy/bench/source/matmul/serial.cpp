#include <bit>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "config.hpp"

namespace {

/**
 * @brief Recursive divide-and-conquer matrix multiplication, powers of 2, only.
 *
 * a00 a01            =  a00 * b00 + a01 * b10,   a00 * b01 + a01 * b11
 * a10 a11            =  a10 * b00 + a11 * b10,   a10 * b01 + a11 * b11
 *           b00 b01
 *           b10 b11
 */
template <typename Add = std::false_type>
void matmul(float *A, float *B, float *R, unsigned n, unsigned s, Add add = {}) {
  //
  if (n * sizeof(float) <= lf::impl::k_cache_line) {
    return multiply(A, B, R, n, s, add);
  }

  LF_ASSERT(std::has_single_bit(n));
  LF_ASSERT(std::has_single_bit(s));

  unsigned m = n / 2;

  unsigned o00 = 0;
  unsigned o01 = m;
  unsigned o10 = m * s;
  unsigned o11 = m * s + m;

  matmul(A + o00, B + o00, R + o00, m, s, add);
  matmul(A + o00, B + o01, R + o01, m, s, add);
  matmul(A + o10, B + o00, R + o10, m, s, add);
  matmul(A + o10, B + o01, R + o11, m, s, add);

  matmul(A + o01, B + o10, R + o00, m, s, std::true_type{});
  matmul(A + o01, B + o11, R + o01, m, s, std::true_type{});
  matmul(A + o11, B + o10, R + o10, m, s, std::true_type{});
  matmul(A + o11, B + o11, R + o11, m, s, std::true_type{});
}

void matmul_serial(benchmark::State &state) {

  state.counters["green_threads"] = 1;
  state.counters["mat NxN"] = matmul_work;

  auto [A, B, C1, C2, n] = matmul_init(matmul_work);

  volatile int m = n;

  for (auto _ : state) {
    matmul(A.get(), B.get(), C1.get(), m, m);
  }

#ifndef LF_NO_CHECK
  iter_matmul(A.get(), B.get(), C2.get(), n);

  if (maxerror(C1.get(), C2.get(), n) > 1e-6f) {
    std::cout << "maxerror: " << maxerror(C1.get(), C2.get(), n) << std::endl;
  }
#endif
}

} // namespace

BENCHMARK(matmul_serial)->UseRealTime();
