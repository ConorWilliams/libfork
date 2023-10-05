#include <algorithm>
#include <span>
#include <thread>

#include <benchmark/benchmark.h>

#include <tbb/task_arena.h>
#include <tbb/task_group.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

auto reduce(std::span<double> x, std::size_t grain_size) -> double {
  //
  if (x.size() <= grain_size) {
    return std::reduce(x.begin(), x.end());
  }

  auto h = x.size() / 2;
  auto t = x.size() - h;

  double a, b;

  tbb::task_group g;

  g.run([&] {
    a = reduce(x.first(h), grain_size);
  });

  b = reduce(x.last(t), grain_size);

  g.wait();

  return a + b;
}

void reduce_tbb(benchmark::State &state) {

  std::size_t n = state.range(0);
  tbb::task_arena arena(n);
  std::vector data = arena.execute([] {
    return to_sum();
  });
  auto grain_size = data.size() / (n * 10);

  volatile double output;

  for (auto _ : state) {
    output = arena.execute([&] {
      return reduce(data, grain_size);
    });
  }

  if (auto exp = std::reduce(data.begin(), data.end()); !is_close(output, exp)) {
    std::cerr << "tbb wrong result: " << output << " != " << exp << std::endl;
  }
}

} // namespace

BENCHMARK(reduce_tbb)->DenseRange(1, num_threads())->UseRealTime();
