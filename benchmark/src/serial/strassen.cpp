#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "matmul.hpp"

import std;

namespace {

inline constexpr unsigned strassen_cutoff = 64;

// Out[i][j] = A[i][j] + B[i][j], all m x m with respective row strides.
void mat_add(float const *A, unsigned sa, float const *B, unsigned sb, float *Out, unsigned so, unsigned m) {
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      Out[i * so + j] = A[i * sa + j] + B[i * sb + j];
    }
  }
}

void mat_sub(float const *A, unsigned sa, float const *B, unsigned sb, float *Out, unsigned so, unsigned m) {
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      Out[i * so + j] = A[i * sa + j] - B[i * sb + j];
    }
  }
}

void naive_multiply(float const *A, unsigned sa, float const *B, unsigned sb, float *C, unsigned sc, unsigned n) {
  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      float sum = 0;
      for (unsigned k = 0; k < n; ++k) {
        sum += A[i * sa + k] * B[k * sb + j];
      }
      C[i * sc + j] = sum;
    }
  }
}

void strassen(float const *A, unsigned sa, float const *B, unsigned sb, float *C, unsigned sc, unsigned n) {

  if (n <= strassen_cutoff) {
    naive_multiply(A, sa, B, sb, C, sc, n);
    return;
  }

  unsigned m = n / 2;

  auto block = [m](auto *p, unsigned s, unsigned i, unsigned j) { return p + i * m * s + j * m; };

  float const *A11 = block(A, sa, 0, 0);
  float const *A12 = block(A, sa, 0, 1);
  float const *A21 = block(A, sa, 1, 0);
  float const *A22 = block(A, sa, 1, 1);
  float const *B11 = block(B, sb, 0, 0);
  float const *B12 = block(B, sb, 0, 1);
  float const *B21 = block(B, sb, 1, 0);
  float const *B22 = block(B, sb, 1, 1);
  float *C11 = block(C, sc, 0, 0);
  float *C12 = block(C, sc, 0, 1);
  float *C21 = block(C, sc, 1, 0);
  float *C22 = block(C, sc, 1, 1);

  std::vector<float> buf(static_cast<std::size_t>(m) * m * 9);
  float *T1 = buf.data();
  float *T2 = T1 + static_cast<std::size_t>(m) * m;
  float *M1 = T2 + static_cast<std::size_t>(m) * m;
  float *M2 = M1 + static_cast<std::size_t>(m) * m;
  float *M3 = M2 + static_cast<std::size_t>(m) * m;
  float *M4 = M3 + static_cast<std::size_t>(m) * m;
  float *M5 = M4 + static_cast<std::size_t>(m) * m;
  float *M6 = M5 + static_cast<std::size_t>(m) * m;
  float *M7 = M6 + static_cast<std::size_t>(m) * m;

  // M1 = (A11 + A22)(B11 + B22)
  mat_add(A11, sa, A22, sa, T1, m, m);
  mat_add(B11, sb, B22, sb, T2, m, m);
  strassen(T1, m, T2, m, M1, m, m);

  // M2 = (A21 + A22) B11
  mat_add(A21, sa, A22, sa, T1, m, m);
  strassen(T1, m, B11, sb, M2, m, m);

  // M3 = A11 (B12 - B22)
  mat_sub(B12, sb, B22, sb, T2, m, m);
  strassen(A11, sa, T2, m, M3, m, m);

  // M4 = A22 (B21 - B11)
  mat_sub(B21, sb, B11, sb, T2, m, m);
  strassen(A22, sa, T2, m, M4, m, m);

  // M5 = (A11 + A12) B22
  mat_add(A11, sa, A12, sa, T1, m, m);
  strassen(T1, m, B22, sb, M5, m, m);

  // M6 = (A21 - A11)(B11 + B12)
  mat_sub(A21, sa, A11, sa, T1, m, m);
  mat_add(B11, sb, B12, sb, T2, m, m);
  strassen(T1, m, T2, m, M6, m, m);

  // M7 = (A12 - A22)(B21 + B22)
  mat_sub(A12, sa, A22, sa, T1, m, m);
  mat_add(B21, sb, B22, sb, T2, m, m);
  strassen(T1, m, T2, m, M7, m, m);

  // Combine.
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < m; ++j) {
      std::size_t k = static_cast<std::size_t>(i) * m + j;
      C11[i * sc + j] = M1[k] + M4[k] - M5[k] + M7[k];
      C12[i * sc + j] = M3[k] + M5[k];
      C21[i * sc + j] = M2[k] + M4[k];
      C22[i * sc + j] = M1[k] - M2[k] + M3[k] + M6[k];
    }
  }
}

template <typename = void>
void strassen_serial(benchmark::State &state) {

  unsigned n = static_cast<unsigned>(state.range(0));
  state.counters["n"] = n;

  auto args = matmul_init(n);
  matmul_iter(args.A.get(), args.B.get(), args.ref.get(), n);

  for (auto _ : state) {
    matmul_zero(args.C.get(), n);
    benchmark::DoNotOptimize(args.A.get());
    benchmark::DoNotOptimize(args.B.get());
    strassen(args.A.get(), n, args.B.get(), n, args.C.get(), n, n);
    benchmark::DoNotOptimize(args.C.get());
  }

  float err = matmul_max_relative_error(args.ref.get(), args.C.get(), n);
  if (err > 1e-3f) {
    state.SkipWithError(std::format("strassen max relative error too high: {}", err));
  }
}

} // namespace

BENCH_ALL(strassen_serial, serial, strassen, strassen)
