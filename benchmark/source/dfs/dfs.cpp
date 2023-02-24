#include <nanobench.h>

#include <thread>

auto omp_dfs(ankerl::nanobench::Bench& bench, std::size_t depth, std::size_t breadth, std::size_t nthreads) -> void;
// auto benchmark_matmul() -> void;

auto benchmark_dfs() -> void {
  ankerl::nanobench::Bench bench;

  bench.title("Fibonacci");
  bench.unit("dfs(8, 8)");
  bench.warmup(100);
  bench.relative(true);
  // bench.epochs(100);
  //   bench.minEpochTime(std::chrono::milliseconds(100));
  // bench.minEpochTime(std::chrono::milliseconds(100));
  // bench.maxEpochTime(std::chrono::milliseconds(1000));
  bench.performanceCounters(true);

  for (int i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    omp_dfs(bench, 6, 6, i);
  }
}