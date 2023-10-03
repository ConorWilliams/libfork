#include <benchmark/benchmark.h>

#include <taskflow/taskflow.hpp>

#include "../util.hpp"
#include "config.hpp"

namespace {

void rec_matmul(double const *A, double const *B, double *C, int m, int n, int p, int ld, tf::Subflow &sbf) {
  if ((m + n + p) <= 64) {

    return multiply(A, B, C, m, n, p, ld);
  }

  if (m >= n && n >= p) {
    int m1 = m >> 1;
    sbf.emplace([=](tf::Subflow &sbf) {
      rec_matmul(A, B, C, m1, n, p, ld, sbf);
    });
    sbf.emplace([=](tf::Subflow &sbf) {
      rec_matmul(A + m1 * ld, B, C + m1 * ld, m - m1, n, p, ld, sbf);
    });
  } else if (n >= m && n >= p) {
    int n1 = n >> 1;
    sbf.emplace([=](tf::Subflow &sbf) {
      rec_matmul(A, B, C, m, n1, p, ld, sbf);
    });
    sbf.emplace([=](tf::Subflow &sbf) {
      rec_matmul(A + n1, B + n1 * ld, C, m, n - n1, p, ld, sbf);
    });
  } else {
    int p1 = p >> 1;
    sbf.emplace([=](tf::Subflow &sbf) {
      rec_matmul(A, B, C, m, n, p1, ld, sbf);
    });
    sbf.emplace([=](tf::Subflow &sbf) {
      rec_matmul(A, B + p1, C + p1, m, n, p - p1, ld, sbf);
    });
  }
  sbf.join();
}

void matmul_taskflow(benchmark::State &state) {

  tf::Executor executor(state.range(0));

  // matmul_args args;

  matmul_args args;

  {
    tf::Taskflow tf;

    tf.emplace([&args](tf::Subflow &sbf) {
      args = matmul_init(matmul_work);
    });

    executor.run(tf).wait();
  }

  auto *A = args.A.get();
  auto *B = args.B.get();
  auto *C1 = args.C1.get();
  auto *C2 = args.C2.get();
  auto n = args.n;

  tf::Taskflow taskflow;

  taskflow.emplace([=](tf::Subflow &sbf) {
    rec_matmul(A, B, C1, n, n, n, n, sbf);
  });

  for (auto _ : state) {
    state.PauseTiming();
    zero(args.C1.get(), args.n);
    state.ResumeTiming();

    executor.run(taskflow).wait();
  }

  iter_matmul(A, B, C2, n);

  if (maxerror(C1, C2, n) > 1e-6) {
    std::cout << "taskflow maxerror: " << maxerror(C1, C2, n) << std::endl;
  }
}

} // namespace

BENCHMARK(matmul_taskflow)->DenseRange(1, num_threads())->UseRealTime();
