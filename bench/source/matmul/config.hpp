#ifndef BE9FE4D3_1849_4309_A6E6_249FEE36A894
#define BE9FE4D3_1849_4309_A6E6_249FEE36A894

#include <atomic>
#include <bit>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

inline void zero(double *A, int n) {
  int i, j;

  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      A[i * n + j] = 0.0;
    }
  }
}

inline void init(double *A, int n) {
  int i, j;

  lf::xoshiro rng{lf::seed, std::random_device{}};

  std::uniform_real_distribution<double> dist{0, 1};

  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      A[i * n + j] = dist(rng);
    }
  }
}

inline auto maxerror(double *A, double *B, int n) -> double {

  double error = 0.0;

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {

      double diff = (A[i * n + j] - B[i * n + j]) / A[i * n + j];

      if (diff < 0) {
        diff = -diff;
      }

      if (diff > error) {
        error = diff;
      }
    }
  }
  return error;
}

inline void iter_matmul(double *A, double *B, double *C, int n) {

  for (int i = 0; i < n; i++) {
    for (int k = 0; k < n; k++) {

      double c = 0.0;

      for (int j = 0; j < n; j++) {
        c += A[i * n + j] * B[j * n + k];
      }

      C[i * n + k] = c;
    }
  }
}

struct matmul_args {
  std::unique_ptr<double[]> A;
  std::unique_ptr<double[]> B;
  std::unique_ptr<double[]> C1;
  std::unique_ptr<double[]> C2;
  int n;
};

inline auto matmul_init(int n) -> matmul_args {
  //
  matmul_args args{
      std::make_unique<double[]>(n * n),
      std::make_unique<double[]>(n * n),
      std::make_unique<double[]>(n * n),
      std::make_unique<double[]>(n * n),
      n,
  };

  init(args.A.get(), n);
  init(args.B.get(), n);

  zero(args.C1.get(), n);
  zero(args.C2.get(), n);

  return args;
}

// --------------------------------------------- //

inline constexpr unsigned int matmul_work = 2048;

static_assert(std::has_single_bit(matmul_work));

inline void multiply(double *A, double *B, double *R, unsigned n, unsigned s, auto add) {
  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      double sum = 0;
      for (unsigned k = 0; k < n; ++k) {
        sum += A[i * s + k] * B[k * s + j];
      }
      if constexpr (add) {
        R[i * s + j] += sum;
      } else {
        R[i * s + j] = sum;
      }
    }
  }
}

#endif /* BE9FE4D3_1849_4309_A6E6_249FEE36A894 */
