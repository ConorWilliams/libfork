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
 * Note according to its own check it is not cforkuting this correctly!
 * I believe this is because it is overflowing.
 */

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

#include "../bench.hpp"

#include "libfork/schedule/busy_pool.hpp"
#include "libfork/task.hpp"

using namespace std;
using namespace lf;

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

template <context Context>
auto matmul(elem_t* A, elem_t* B, elem_t* C, size_t m, size_t n, size_t p, size_t ld, bool add) -> basic_task<void, Context> {
  if ((m + n + p) <= 64) {
    if (add) {
      for (size_t i = 0; i < m; ++i) {
        for (size_t k = 0; k < p; ++k) {
          double c = 0.0;
          for (size_t j = 0; j < n; ++j)
            c += A[i * ld + j] * B[j * ld + k];
          C[i * ld + k] += c;
        }
      }
    } else {
      for (size_t i = 0; i < m; ++i) {
        for (size_t k = 0; k < p; ++k) {
          double c = 0.0;
          for (size_t j = 0; j < n; ++j)
            c += A[i * ld + j] * B[j * ld + k];
          C[i * ld + k] = c;
        }
      }
    }
    co_return;
  }

  if (m >= n && n >= p) {
    size_t m1 = m >> 1;

    co_await matmul<Context>(A, B, C, m1, n, p, ld, add).fork();
    co_await matmul<Context>(A + m1 * ld, B, C + m1 * ld, m - m1, n, p, ld, add);

  } else if (n >= m && n >= p) {
    size_t n1 = n >> 1;

    co_await matmul<Context>(A, B, C, m, n1, p, ld, add).fork();
    co_await matmul<Context>(A + n1, B + n1 * ld, C, m, n - n1, p, ld, true);
  } else {
    size_t p1 = p >> 1;

    co_await matmul<Context>(A, B, C, m, n, p1, ld, add).fork();
    co_await matmul<Context>(A, B + p1, C + p1, m, n, p - p1, ld, add);
  }

  co_await join();
}

template <context Context>
auto test(elem_t* A, elem_t* B, elem_t* C, size_t n) -> basic_task<void, Context> {
  co_await matmul<Context>(A, B, C, n, n, n, n, 0);
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

    auto pool = busy_pool{num_threads};

    bench([&] {
      pool.schedule(test<busy_pool::context>(A, B, C, n));
    });

    int res = check(A, B, C, n);

    delete[] C;
    delete[] B;
    delete[] A;

    return res;
  });
}

int main(int argc, char* argv[]) {
  run("fork-matmul-n=010", 8);
  run("fork-matmul-n=030", 32);
  run("fork-matmul-n=050", 64);
  run("fork-matmul-n=100", 128);
  run("fork-matmul-n=300", 256);
  run("fork-matmul-n=500", 512);
  run("fork-matmul-n=700", 1024);
  return 0;
}