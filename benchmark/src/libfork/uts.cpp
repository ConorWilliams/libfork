#include <benchmark/benchmark.h>

#include "common.hpp"
#include "helpers.hpp"
#include "uts.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <utility>
#include <vector>

import libfork;

// === Coroutine

namespace {

// TODO: try a version that uses try_fork

struct uts_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, int depth, Node *parent) -> lf::task<result, Context> {

    result r{.maxdepth = static_cast<counter_t>(depth), .size = counter_t{1}, .leaves = counter_t{0}};

    int num_children = uts_numChildren(parent);
    int child_type = uts_childType(parent);

    parent->numChildren = num_children;

    if (num_children > 0) {
      std::vector<pair> cs(static_cast<std::size_t>(num_children));

      auto sc = co_await lf::scope();

      for (std::size_t i = 0; i < static_cast<std::size_t>(num_children); ++i) {
        cs[i].child.type = child_type;
        cs[i].child.height = parent->height + 1;
        cs[i].child.numChildren = -1;

        for (int j = 0; j < computeGranularity; ++j) {
          rng_spawn(parent->state.state, cs[i].child.state.state, static_cast<int>(i));
        }

        if (i + 1 == static_cast<std::size_t>(num_children)) {
          co_await sc.call(&cs[i].res, uts_fn{}, depth + 1, &cs[i].child);
        } else {
          co_await sc.fork(&cs[i].res, uts_fn{}, depth + 1, &cs[i].child);
        }
      }

      co_await sc.join();

      for (auto &&elem : cs) {
        r.maxdepth = std::max(r.maxdepth, elem.res.maxdepth);
        r.size += elem.res.size;
        r.leaves += elem.res.leaves;
      }
    } else {
      r.leaves = 1;
    }

    co_return r;
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state, uts_tree tree) {
  setup_tree(tree);
  auto expect = expected_result(tree);

  std::size_t threads = static_cast<std::size_t>(state.range(0));
  state.counters["p"] = static_cast<double>(threads);
  state.SetComplexityN(static_cast<benchmark::IterationCount>(threads));

  Sch scheduler = Sch{threads};

  for (auto _ : state) {
    Node root;
    uts_initRoot(&root, type);
    lf::receiver recv = lf::schedule(scheduler, uts_fn{}, 0, &root);
    result r = std::move(recv).get();

    if (r != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", r, expect));
      break;
    }

    benchmark::DoNotOptimize(r);
  }
}

} // namespace

LIBFORK_UTS_BENCH_ALL_MT(run, mono_busy_pool)
LIBFORK_UTS_BENCH_ALL_MT(run, poly_busy_pool)
