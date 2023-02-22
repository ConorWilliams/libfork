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
 */

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

// NOLINTBEGIN

using namespace std;
using namespace chrono;

using elem_t = double;

inline uint32_t xorshift_rand() {
  static uint32_t x = 2463534242;
  x ^= x >> 13;
  x ^= x << 17;
  x ^= x >> 5;
  return x;
}

inline void zero(elem_t* A, size_t n) {
  for (size_t i = 0; i < n; ++i)
    for (size_t j = 0; j < n; ++j)
      A[i * n + j] = 0.0;
}

inline void fill(elem_t* A, size_t n) {
  for (size_t i = 0; i < n; ++i)
    for (size_t j = 0; j < n; ++j)
      A[i * n + j] = xorshift_rand() % n;
}

inline double maxerror(elem_t* A, elem_t* B, size_t n) {
  size_t i, j;
  double error = 0.0;

  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      double diff = (A[i * n + j] - B[i * n + j]) / A[i * n + j];
      if (diff < 0)
        diff = -diff;
      if (diff > error)
        error = diff;
    }
  }

  return error;
}

inline bool check(elem_t* A, elem_t* B, elem_t* C, size_t n) {
  elem_t tr_C = 0;
  elem_t tr_AB = 0;
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = 0; j < n; ++j)
      tr_AB += A[i * n + j] * B[j * n + i];
    tr_C += C[i * n + i];
  }

  return fabs(tr_AB - tr_C) < 1e-3;
}

// NOLINTEND
