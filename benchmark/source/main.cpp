#include <nanobench.h>
#include <thread>

#include "libfork/schedule/busy_pool.hpp"
#include "libfork/task.hpp"

auto benchmark_fib() -> void;
auto benchmark_reduce() -> void;

// noop task
static auto noop() -> lf::basic_task<void, lf::busy_pool::context> {
  co_return;
}

auto main() -> int {
  //
  ankerl::nanobench::Bench bench;

  bench.title("Noop");
  bench.relative(true);
  // bench.epochs(100);
  bench.minEpochTime(std::chrono::milliseconds(100));
  bench.performanceCounters(true);

  for (std::size_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    //

    lf::busy_pool pool{i};

    bench.run("noop " + std::to_string(i) + " threads", [&] {
      pool.schedule(noop());
    });
  }

  benchmark_reduce();
  benchmark_fib();
  return 0;
}