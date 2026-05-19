#include <benchmark/benchmark.h>

#include "skynet.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

constexpr auto n = static_cast<std::size_t>(skynet_branching - 1);

struct skynet_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, std::int64_t num, int depth) -> lf::task<std::int64_t, Context> {

    if (depth == 0) {
      co_return num;
    }

    auto const sub = skynet_leaves(depth - 1);
    std::array<std::int64_t, skynet_branching> children{};

    auto sc = co_await lf::scope();

    for (std::size_t i = 0; i < n; ++i) {
      co_await sc.fork(&children[i], skynet_fn{}, num + static_cast<std::int64_t>(i) * sub, depth - 1);
    }
    co_await sc.call(&children[n], skynet_fn{}, num + static_cast<std::int64_t>(n) * sub, depth - 1);

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
