#include <algorithm>
#include <span>
#include <thread>

#include <benchmark/benchmark.h>

#include <taskflow/taskflow.hpp>

#include "../util.hpp"
#include "config.hpp"

namespace {

auto reduce(std::span<double> data, std::size_t n, tf::Subflow &sbf) -> double {

  if (data.size() <= n) {
    return std::reduce(data.begin(), data.end());
  }

  auto h = data.size() / 2;
  auto t = data.size() - h;

  double a, b;

  sbf.emplace([&](tf::Subflow &sbf) {
    a = reduce(data.first(h), n, sbf);
  });

  sbf.emplace([&](tf::Subflow &sbf) {
    b = reduce(data.last(t), n, sbf);
  });

  sbf.join();

  return a + b;
}

auto alloc(tf::Subflow &sbf) -> std::vector<double> { return to_sum(); }

void reduce_taskflow(benchmark::State &state) {

  std::size_t n = state.range(0);

  tf::Executor executor(n);

  std::vector<double> data;

  {
    tf::Taskflow alloca;

    alloca.emplace([&data](tf::Subflow &sbf) {
      data = alloc(sbf);
    });

    executor.run(alloca).wait();
  }

  auto grain_size = data.size() / (n * 10);

  volatile double output;

  tf::Taskflow taskflow;

  taskflow.emplace([&](tf::Subflow &sbf) {
    output = reduce(data, grain_size, sbf);
  });

  for (auto _ : state) {
    executor.run(taskflow).wait();
  }

  if (auto exp = std::reduce(data.begin(), data.end()); !is_close(output, exp)) {
    std::cerr << "taskflow wrong result: " << output << " != " << exp << std::endl;
  }
}

} // namespace

BENCHMARK(reduce_taskflow)->DenseRange(1, num_threads())->UseRealTime();
