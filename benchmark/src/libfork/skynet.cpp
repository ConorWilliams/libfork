#include <benchmark/benchmark.h>

#include "skynet.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct skynet_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, std::int64_t num, int depth) -> lf::task<std::int64_t, Context> {
    if (depth == 0) {
      co_return num;
    }

    std::int64_t sub = skynet_leaves(depth - 1);
    std::array<std::int64_t, skynet_branching> results{};

    auto sc = co_await lf::scope();
    for (int i = 0; i < skynet_branching; ++i) {
      auto child_num = num + static_cast<std::int64_t>(i) * sub;
      if (i + 1 == skynet_branching) {
        co_await sc.call(&results[static_cast<std::size_t>(i)], skynet_task{}, child_num, depth - 1);
      } else {
        co_await sc.fork(&results[static_cast<std::size_t>(i)], skynet_task{}, child_num, depth - 1);
      }
    }
    co_await sc.join();

    std::int64_t sum = 0;
    for (auto result : results) {
      sum += result;
    }
    co_return sum;
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);
  lf_bench::report_threads(state, threads);

  run_skynet(state, [&](std::int64_t num, int depth) {
    return lf::schedule(scheduler, skynet_task{}, num, depth).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, skynet, skynet, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, skynet, skynet, poly_busy_pool)
