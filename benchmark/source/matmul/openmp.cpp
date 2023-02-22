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

#include <omp.h>

#include "common.hpp"

namespace {

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
#pragma omp task untied shared(A, B, C)
    matmul(A, B, C, m1, n, p, ld, add);

    matmul(A + m1 * ld, B, C + m1 * ld, m - m1, n, p, ld, add);

  } else if (n >= m && n >= p) {
    size_t n1 = n >> 1;

#pragma omp task untied shared(A, B, C)
    matmul(A, B, C, m, n1, p, ld, add);

    matmul(A + n1, B + n1 * ld, C, m, n - n1, p, ld, true);

  } else {
    size_t p1 = p >> 1;
#pragma omp task untied shared(A, B, C)
    matmul(A, B, C, m, n, p1, ld, add);

    matmul(A, B + p1, C + p1, m, n, p - p1, ld, add);
  }

#pragma omp taskwait
}

void test(elem_t* A, elem_t* B, elem_t* C, size_t n) {
#pragma omp task untied shared(A, B, C, n)
  matmul(A, B, C, n, n, n, n, 0);
#pragma omp taskwait
}

}  // namespace

int main(int argc, char* argv[]) {
  elem_t *A, *B, *C;
  size_t n = 3000;
  size_t nthreads = 0;

  if (argc >= 2)
    nthreads = atoi(argv[1]);
  if (argc >= 3)
    n = atoi(argv[2]);
  if (nthreads == 0)
    nthreads = thread::hardware_concurrency();

  A = new elem_t[n * n];
  B = new elem_t[n * n];
  C = new elem_t[n * n];

  fill(A, n);
  fill(B, n);
  zero(C, n);

  auto start = system_clock::now();

  omp_set_dynamic(0);
  omp_set_num_threads(nthreads);

#pragma omp parallel shared(A, B, C, n)
#pragma omp single
  test(A, B, C, n);

  auto stop = system_clock::now();

  cout << "Scheduler:  openmp\n";
  cout << "Benchmark:  matmul\n";
  cout << "Threads:    " << nthreads << "\n";
  cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
  cout << "Input:      " << n << "\n";
  cout << "Output:     " << check(A, B, C, n) << "\n";

  delete[] C;
  delete[] B;
  delete[] A;
  return 0;
}