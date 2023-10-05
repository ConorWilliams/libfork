#include <algorithm>
#include <span>
#include <thread>

#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include "../util.hpp"
#include "config.hpp"

namespace {

inline constexpr lf::async reduce =
    [](lf::first_arg auto reduce, std::span<double> data, std::size_t n) -> lf::task<double> {
  if (data.size() <= n) {
    co_return std::reduce(data.begin(), data.end());
  }

  auto h = data.size() / 2;
  auto t = data.size() - h;

  double a, b;

  co_await lf::fork(a, reduce)(data.first(h), n);
  co_await lf::call(b, reduce)(data.last(t), n);

  co_await lf::join;

  co_return a + b;
};

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void reduce_libfork(benchmark::State &state) {

  std::size_t n = state.range(0);
  Sch sch(n, Strategy);
  std::vector data = lf::sync_wait(sch, LF_LIFT2(to_sum));
  auto grain_size = data.size() / (n * 10);

  volatile double output;

  for (auto _ : state) {
    output = lf::sync_wait(sch, reduce, data, grain_size);
  }

  if (auto exp = std::reduce(data.begin(), data.end()); !is_close(output, exp)) {
    std::cerr << "lf wrong result: " << output << " != " << exp << std::endl;
  }
}

} // namespace

using namespace lf;

BENCHMARK(reduce_libfork<lazy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();

BENCHMARK(reduce_libfork<lazy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();

BENCHMARK(reduce_libfork<busy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();

BENCHMARK(reduce_libfork<busy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();