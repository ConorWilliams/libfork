/*
 * Rectangular matrix multiplication.
 *
 * Adapted from Cilk 5.4.3 example
 *
 * https://bradley.csail.mit.edu/svn/repos/cilk/5.4.3/examples/matmul.cilk;
 * See the paper ``Cache-Oblivious Algorithms'', by
 * Matteo Frigo, Charles E. Leiserson, Harald Prokop, and
 * Sridhar Ramachandran, FOCS 1999, for an explanation of
 * why this algorithm is good for caches.
 *
 * Note according to its own check it is not computing this correctly!
 * I belive this is because it is overflowing.
 */

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

#include "../bench.hpp"

using namespace std;

using elem_t = float;

inline auto xorshift_rand() -> uint32_t {
  static uint32_t x = 2463534242;
  x ^= x >> 13;
  x ^= x << 17;
  x ^= x >> 5;
  return x;
}

void zero(elem_t* A, size_t n) {
  for (size_t i = 0; i < n; ++i)
    for (size_t j = 0; j < n; ++j)
      A[i * n + j] = 0.0;
}

void fill(elem_t* A, size_t n) {
  for (size_t i = 0; i < n; ++i)
    for (size_t j = 0; j < n; ++j)
      A[i * n + j] = xorshift_rand() % n;
}

bool check(elem_t* A, elem_t* B, elem_t* C, size_t n) {
  double tr_C = 0;
  double tr_AB = 0;
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = 0; j < n; ++j)
      tr_AB += A[i * n + j] * B[j * n + i];
    tr_C += C[i * n + i];
  }

  return fabs(tr_AB - tr_C) < 1e-3;
}

void matmul(elem_t* A, elem_t* B, elem_t* C, size_t m, size_t n, size_t p, size_t ld, bool add) {
  if ((m + n + p) <= 64) {
    if (add) {
      for (size_t i = 0; i < m; ++i) {
        for (size_t k = 0; k < p; ++k) {
          elem_t c = 0.0;
          for (size_t j = 0; j < n; ++j)
            c += A[i * ld + j] * B[j * ld + k];
          C[i * ld + k] += c;
        }
      }
    } else {
      for (size_t i = 0; i < m; ++i) {
        for (size_t k = 0; k < p; ++k) {
          elem_t c = 0.0;
          for (size_t j = 0; j < n; ++j)
            c += A[i * ld + j] * B[j * ld + k];
          C[i * ld + k] = c;
        }
      }
    }

    return;
  }

  if (m >= n && n >= p) {
    size_t m1 = m >> 1;
#pragma omp task shared(A, B, C)
    matmul(A, B, C, m1, n, p, ld, add);

    matmul(A + m1 * ld, B, C + m1 * ld, m - m1, n, p, ld, add);
  } else if (n >= m && n >= p) {
    size_t n1 = n >> 1;
#pragma omp task shared(A, B, C)
    matmul(A, B, C, m, n1, p, ld, add);

    matmul(A + n1, B + n1 * ld, C, m, n - n1, p, ld, true);
  } else {
    size_t p1 = p >> 1;
#pragma omp task shared(A, B, C)
    matmul(A, B, C, m, n, p1, ld, add);

    matmul(A, B + p1, C + p1, m, n, p - p1, ld, add);
  }

#pragma omp taskwait
}

void test(elem_t* A, elem_t* B, elem_t* C, size_t n) {
#pragma omp task shared(A, B, C, n)
  matmul(A, B, C, n, n, n, n, 0);
#pragma omp taskwait
}

void run(std::string name, size_t n) {
  benchmark(name, [&](std::size_t num_threads, auto&& bench) {
    // Set up
    elem_t *A, *B, *C;

    A = new elem_t[n * n];
    B = new elem_t[n * n];
    C = new elem_t[n * n];

    fill(A, n);
    fill(B, n);
    zero(C, n);

#pragma omp parallel num_threads(num_threads)
#pragma omp single nowait
    bench([&] {
      test(A, B, C, n);
    });

    int res = check(A, B, C, n);

    delete[] C;
    delete[] B;
    delete[] A;

    return check;
  });
}

int main(int argc, char* argv[]) {
  run("libfork, matmul n=10", 10);
  run("libfork, matmul n=30", 30);
  run("libfork, matmul n=50", 50);
  run("libfork, matmul n=100", 100);
  run("libfork, matmul n=300", 300);
  run("libfork, matmul n=500", 500);
  run("libfork, matmul n=1000", 1000);
  return 0;
}