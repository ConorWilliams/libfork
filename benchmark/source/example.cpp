
#include <chrono>
#include <string>
#include <thread>

#include <nanobench.h>

#include <tbb/task_arena.h>
#include <tbb/task_group.h>

#include "libfork/schedule/busy_pool.hpp"
#include "libfork/task.hpp"

namespace {

auto fib(int n) -> int {
  if (n < 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

template <lf::context Context>
auto libfork(int n) -> lf::basic_task<int, Context> {
  if (n < 2) {
    co_return n;
  }
  auto a = co_await libfork<Context>(n - 1).fork();
  auto b = co_await libfork<Context>(n - 2);

  co_await lf::join();

  co_return *a + b;
}

auto omp(int n) -> int {
  if (n < 2) {
    return n;
  }

  int a, b;

#pragma omp task untied shared(a)
  a = omp(n - 1);

  b = omp(n - 2);

#pragma omp taskwait

  return a + b;
}

int fib_tbb(int n) {
  if (n < 2) {
    return n;
  }
  int x, y;

  tbb::task_group g;

  g.run([&] {
    x = fib_tbb(n - 1);
  });

  y = fib_tbb(n - 2);

  g.wait();

  return x + y;
}

}  // namespace

auto main() -> int {
  //
  ankerl::nanobench::Bench bench;

  int volatile fib_number = 20;

  bench.title("Fibbonaci");
  bench.unit("fib(" + std::to_string(fib_number) + ")");
  bench.warmup(100);
  bench.relative(true);
  bench.minEpochIterations(100);
  bench.minEpochTime(std::chrono::milliseconds(100));
  bench.performanceCounters(true);

  for (int i = 1; i <= std::thread::hardware_concurrency(); ++i) {
#pragma omp parallel num_threads(i)
#pragma omp single nowait
    {
      bench.run("openMP " + std::to_string(i) + " threads", [&] {
        ankerl::nanobench::doNotOptimizeAway(omp(fib_number));
      });
    }
  }

  for (std::size_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    //
    lf::busy_pool pool{i};

    bench.run("busy_pool " + std::to_string(i) + " threads", [&] {
      ankerl::nanobench::doNotOptimizeAway(pool.schedule(libfork<lf::busy_pool::context>(fib_number)));
    });
  }

  for (int i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    //

    tbb::task_arena limited(i);

    limited.execute([&] {
      bench.run("intel TBB " + std::to_string(i) + " threads", [&] {
        ankerl::nanobench::doNotOptimizeAway(fib_tbb(fib_number));
      });
    });
  }

  return 0;
}