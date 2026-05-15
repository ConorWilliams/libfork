#include <benchmark/benchmark.h>

#include "skynet.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct skynet_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, std::int64_t num, int depth) -> lf::task<std::int64_t, Context> {
    if (depth == 0) {
      co_return num;
    }

    auto const sub = skynet_leaves(depth - 1);
    std::array<std::int64_t, skynet_branching> children{};

    auto sc = co_await lf::scope();

    for (int i = 0; i < skynet_branching; ++i) {
      auto const child_num = num + i * sub;
      auto *result = &children[static_cast<std::size_t>(i)];

      if (i + 1 == skynet_branching) {
        co_await sc.call(result, skynet_fn{}, child_num, depth - 1);
      } else {
        co_await sc.fork(result, skynet_fn{}, child_num, depth - 1);
      }
    }

    co_await sc.join();

    co_return std::accumulate(children.begin(), children.end(), std::int64_t{0});
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {

  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_skynet(state, threads, [&](std::int64_t num, int depth) -> std::int64_t {
    return lf::schedule(scheduler, skynet_fn{}, num, depth).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, skynet, skynet, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, skynet, skynet, poly_busy_pool)
