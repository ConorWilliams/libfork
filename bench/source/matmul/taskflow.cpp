#include <benchmark/benchmark.h>

#include <taskflow/taskflow.hpp>

#include "../util.hpp"
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
void matmul(double *A, double *B, double *R, unsigned n, unsigned s, Add add, tf::Subflow &sbf) {
  //
  if (n * sizeof(double) <= lf::impl::k_cache_line) {
    return multiply(A, B, R, n, s, add);
  }

  LF_ASSERT(std::has_single_bit(n));
  LF_ASSERT(std::has_single_bit(s));

  unsigned m = n / 2;

  unsigned o00 = 0;
  unsigned o01 = m;
  unsigned o10 = m * s;
  unsigned o11 = m * s + m;

  sbf.emplace([=](tf::Subflow &sbf) {
    matmul(A + o00, B + o00, R + o00, m, s, add, sbf);
  });
  sbf.emplace([=](tf::Subflow &sbf) {
    matmul(A + o00, B + o01, R + o01, m, s, add, sbf);
  });
  sbf.emplace([=](tf::Subflow &sbf) {
    matmul(A + o10, B + o00, R + o10, m, s, add, sbf);
  });
  sbf.corun([=](tf::Subflow &sbf) {
    matmul(A + o10, B + o01, R + o11, m, s, add, sbf);
  });

  sbf.join();

  sbf.reset();

  sbf.emplace([=](tf::Subflow &sbf) {
    matmul(A + o01, B + o10, R + o00, m, s, std::true_type{}, sbf);
  });
  sbf.emplace([=](tf::Subflow &sbf) {
    matmul(A + o01, B + o11, R + o01, m, s, std::true_type{}, sbf);
  });
  sbf.emplace([=](tf::Subflow &sbf) {
    matmul(A + o11, B + o10, R + o10, m, s, std::true_type{}, sbf);
  });
  sbf.corun([=](tf::Subflow &sbf) {
    matmul(A + o11, B + o11, R + o11, m, s, std::true_type{}, sbf);
  });

  sbf.join();
}

void matmul_taskflow(benchmark::State &state) {

  tf::Executor executor(state.range(0));

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

  taskflow.emplace([&](tf::Subflow &sbf) {
    matmul(A, B, C1, n, n, std::false_type{}, sbf);
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
