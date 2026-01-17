#include "../util.hpp"
#include "config.hpp"
#include <benchmark/benchmark.h>

#include "tmc/all_headers.hpp"

namespace {

using namespace tmc;

using mat = float *;

/**
 * @brief Recursive divide-and-conquer matrix multiplication, powers of 2, only.
 *
 * a00 a01            =  a00 * b00 + a01 * b10,   a00 * b01 + a01 * b11
 * a10 a11            =  a10 * b00 + a11 * b10,   a10 * b01 + a11 * b11
 *           b00 b01
 *           b10 b11
 */
auto matmul(mat A, mat B, mat R, unsigned n, unsigned s, auto add) -> task<void> {
  //
  if (n * sizeof(float) <= lf::impl::k_cache_line) {
    co_return multiply(A, B, R, n, s, add);
  }

  LF_ASSERT(std::has_single_bit(n));
  LF_ASSERT(std::has_single_bit(s));

  unsigned m = n / 2;

  unsigned o00 = 0;
  unsigned o01 = m;
  unsigned o10 = m * s;
  unsigned o11 = m * s + m;

  {
    auto a = spawn(matmul(A + o00, B + o00, R + o00, m, s, add)).run_early();
    auto b = spawn(matmul(A + o00, B + o01, R + o01, m, s, add)).run_early();
    auto c = spawn(matmul(A + o10, B + o00, R + o10, m, s, add)).run_early();
    /* */ co_await matmul(A + o10, B + o01, R + o11, m, s, add);

    co_await a;
    co_await b;
    co_await c;
  }

  {
    auto a = spawn(matmul(A + o01, B + o10, R + o00, m, s, std::true_type{})).run_early();
    auto b = spawn(matmul(A + o01, B + o11, R + o01, m, s, std::true_type{})).run_early();
    auto c = spawn(matmul(A + o11, B + o10, R + o10, m, s, std::true_type{})).run_early();
    /* */ co_await matmul(A + o11, B + o11, R + o11, m, s, std::true_type{});

    co_await a;
    co_await b;
    co_await c;
  }
};

void matmul_tmc(benchmark::State &state) {

  state.counters["green_threads"] = state.range(0);
  state.counters["mat NxN"] = matmul_work;

  tmc::cpu_executor().set_thread_count(state.range(0)).init();

  auto [A, B, C1, C2, n] = matmul_init(matmul_work);

  for (auto _ : state) {
    tmc::post_waitable(tmc::cpu_executor(), matmul(A.get(), B.get(), C1.get(), n, n, std::false_type{}), 0)
        .get();
  }

  tmc::cpu_executor().teardown();

#ifndef LF_NO_CHECK
  iter_matmul(A.get(), B.get(), C2.get(), n);

  if (maxerror(C1.get(), C2.get(), n) > 1e-6) {
    std::cout << "lf maxerror: " << maxerror(C1.get(), C2.get(), n) << std::endl;
  }
#endif
}

} // namespace

using namespace lf;

// BENCHMARK(matmul_libfork<unit_pool, numa_strategy::seq>)->DenseRange(1, 1)->UseRealTime();
// BENCHMARK(matmul_libfork<debug_pool, numa_strategy::seq>)->DenseRange(1, 1)->UseRealTime();

BENCHMARK(matmul_tmc)->Apply(targs)->UseRealTime();
