#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "../util.hpp"
#include "config.hpp"

namespace {

using namespace lf;

constexpr async rec_matmul =
    [](auto rec_matmul, double const *A, double const *B, double *C, int m, int n, int p, int ld) -> task<> {
  if ((m + n + p) <= matmul_grain) {
    co_return multiply(A, B, C, m, n, p, ld);
  }

  if (m >= n && n >= p) {
    int m1 = m >> 1;
    co_await lf::fork(rec_matmul)(A, B, C, m1, n, p, ld);
    co_await lf::call(rec_matmul)(A + m1 * ld, B, C + m1 * ld, m - m1, n, p, ld);
  } else if (n >= m && n >= p) {
    int n1 = n >> 1;
    co_await lf::fork(rec_matmul)(A, B, C, m, n1, p, ld);
    co_await lf::call(rec_matmul)(A + n1, B + n1 * ld, C, m, n - n1, p, ld);
  } else {
    int p1 = p >> 1;
    co_await lf::fork(rec_matmul)(A, B, C, m, n, p1, ld);
    co_await lf::call(rec_matmul)(A, B + p1, C + p1, m, n, p - p1, ld);
  }
  co_await lf::join;
};

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void matmul_libfork(benchmark::State &state) {

  Sch sch = [&] {
    if constexpr (std::constructible_from<Sch, int>) {
      return Sch(state.range(0));
    } else {
      return Sch{};
    }
  }();

  auto [A, B, C1, C2, n] = lf::sync_wait(sch, LF_LIFT2(matmul_init), matmul_work);

  for (auto _ : state) {

    state.PauseTiming();
    zero(C1.get(), n);
    state.ResumeTiming();

    lf::sync_wait(sch, rec_matmul, A.get(), B.get(), C1.get(), n, n, n, n);
  }

  iter_matmul(A.get(), B.get(), C2.get(), n);

  if (maxerror(C1.get(), C2.get(), n) > 1e-6) {
    std::cout << "lf maxerror: " << maxerror(C1.get(), C2.get(), n) << std::endl;
  }
}

} // namespace

using namespace lf;

// BENCHMARK(matmul_libfork<unit_pool, numa_strategy::seq>)->DenseRange(1, 1)->UseRealTime();
// BENCHMARK(matmul_libfork<debug_pool, numa_strategy::seq>)->DenseRange(1, 1)->UseRealTime();

// BENCHMARK(matmul_libfork<lazy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
// BENCHMARK(matmul_libfork<lazy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();

BENCHMARK(matmul_libfork<busy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
// BENCHMARK(matmul_libfork<busy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();