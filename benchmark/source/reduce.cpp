
#include <algorithm>
#include <chrono>
#include <numeric>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include <nanobench.h>

#include <tbb/task_arena.h>
#include <tbb/task_group.h>

#include "libfork/schedule/busy_pool.hpp"
#include "libfork/task.hpp"

namespace {

template <lf::context Context = lf::busy_pool::context>
auto libfork_r(std::span<int> x, std::size_t block) -> lf::basic_task<int, Context> {
  if (x.size() <= block) {
    co_return std::reduce(x.begin(), x.end());
  }

  auto h = x.size() / 2;
  auto t = x.size() - h;

  auto a = co_await libfork_r<Context>(x.first(h), block).fork();
  auto b = co_await libfork_r<Context>(x.last(h), block);

  co_await lf::join();

  co_return *a + b;
}

auto omp_r(std::span<int> x, std::size_t block) -> int {
  if (x.size() <= block) {
    return std::reduce(x.begin(), x.end());
  }

  auto h = x.size() / 2;
  auto t = x.size() - h;

  int a, b;

#pragma omp task shared(a)
  a = omp_r(x.first(h), block);

  b = omp_r(x.last(t), block);

#pragma omp taskwait

  return a + b;
}

int tbb_r(std::span<int> x, std::size_t block) {
  if (x.size() <= block) {
    return std::reduce(x.begin(), x.end());
  }

  auto h = x.size() / 2;
  auto t = x.size() - h;

  int a, b;

  tbb::task_group g;

  g.run([&] {
    a = tbb_r(x.first(h), block);
  });

  b = omp_r(x.last(t), block);

  g.wait();

  return a + b;
}

}  // namespace

auto benchmark_reduce() -> void {
  //
  ankerl::nanobench::Bench bench;

  constexpr int fib_number = 25;

  bench.title("Reduce");
  bench.warmup(100);
  bench.relative(true);
  // bench.epochs(100);
  bench.minEpochTime(std::chrono::milliseconds(100));
  // bench.minEpochTime(std::chrono::milliseconds(100));
  // bench.maxEpochTime(std::chrono::milliseconds(1000));
  bench.performanceCounters(true);

  std::size_t chunk = 100'000;

  std::vector<int> x(1 * 2 * 3 * 4 * 10'00'000);

  std::iota(x.begin(), x.end(), 0);

  auto correct = std::reduce(x.begin(), x.end());

  bench.run("serial threads", [&] {
    ankerl::nanobench::doNotOptimizeAway(std::reduce(x.begin(), x.end()));
  });

  for (std::size_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    //

    lf::busy_pool pool{i};

    bench.run("busy_pool " + std::to_string(i) + " threads", [&] {
      auto y = pool.schedule(libfork_r(x, x.size() / i));

      ankerl::nanobench::doNotOptimizeAway(y);

      if (y != correct) {
        throw std::runtime_error("pool failed");
      }
    });
  }

  for (std::size_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
//
#pragma omp parallel num_threads(i)
#pragma omp single nowait
    bench.run("openMP tasking" + std::to_string(i) + " threads", [&] {
      ankerl::nanobench::doNotOptimizeAway(omp_r(x, x.size() / i));
    });
  }

  for (int i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    //

    tbb::task_arena limited(i);

    limited.execute([&] {
      bench.run("intel TBB " + std::to_string(i) + " threads", [&] {
        auto y = tbb_r(x, x.size() / i);
        ankerl::nanobench::doNotOptimizeAway(x);
        if (y != correct) {
          throw std::runtime_error("tbb failed");
        }
      });
    });
  }

  for (std::size_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    //
    bench.run("openMP raw " + std::to_string(i) + " threads", [&] {
      int sum = 0;
#pragma omp parallel for reduction(+ : sum) num_threads(i)
      for (int i = 0; i < x.size(); i++) {
        sum = sum + x[i];
      }
      ankerl::nanobench::doNotOptimizeAway(sum);
    });
  }
}