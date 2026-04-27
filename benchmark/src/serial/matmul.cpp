#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "matmul.hpp"

import std;

namespace {

template <bool Add>
void matmul_dc(float const *A, float const *B, float *R, unsigned n, unsigned s) {
  if (n <= matmul_basecase) {
    matmul_basecase_multiply<Add>(A, B, R, n, s);
    return;
  }

  unsigned m = n / 2;

  unsigned o00 = 0;
  unsigned o01 = m;
  unsigned o10 = m * s;
  unsigned o11 = m * s + m;

  matmul_dc<Add>(A + o00, B + o00, R + o00, m, s);
  matmul_dc<Add>(A + o00, B + o01, R + o01, m, s);
  matmul_dc<Add>(A + o10, B + o00, R + o10, m, s);
  matmul_dc<Add>(A + o10, B + o01, R + o11, m, s);

  matmul_dc<true>(A + o01, B + o10, R + o00, m, s);
  matmul_dc<true>(A + o01, B + o11, R + o01, m, s);
  matmul_dc<true>(A + o11, B + o10, R + o10, m, s);
  matmul_dc<true>(A + o11, B + o11, R + o11, m, s);
}

template <typename = void>
void matmul_serial(benchmark::State &state) {

  unsigned n = static_cast<unsigned>(state.range(0));
  state.counters["n"] = n;

  auto args = matmul_init(n);
  matmul_iter(args.A.get(), args.B.get(), args.ref.get(), n);

  for (auto _ : state) {
    matmul_zero(args.C.get(), n);
    benchmark::DoNotOptimize(args.A.get());
    benchmark::DoNotOptimize(args.B.get());
    matmul_dc<false>(args.A.get(), args.B.get(), args.C.get(), n, n);
    benchmark::DoNotOptimize(args.C.get());
  }

  float err = matmul_max_relative_error(args.ref.get(), args.C.get(), n);
  if (err > 1e-5f) {
    state.SkipWithError(std::format("matmul max relative error too high: {}", err));
  }
}

} // namespace

BENCH_ALL(matmul_serial, serial, matmul, matmul)
