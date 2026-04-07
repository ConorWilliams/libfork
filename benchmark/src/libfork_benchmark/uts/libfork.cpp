#include <benchmark/benchmark.h>

#include "libfork_benchmark/common.hpp"
#include "libfork_benchmark/uts/uts.hpp"

import libfork;
import std;

// === Coroutine

namespace {

struct uts_fn {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, int depth, Node *parent) -> lf::task<result, Context> {
    result r{static_cast<counter_t>(depth), counter_t{1}, counter_t{0}};

    int num_children = uts_numChildren(parent);
    int child_type = uts_childType(parent);

    parent->numChildren = num_children;

    if (num_children > 0) {
      std::vector<pair> cs(static_cast<std::size_t>(num_children));

      for (std::size_t i = 0; i < static_cast<std::size_t>(num_children); ++i) {
        cs[i].child.type = child_type;
        cs[i].child.height = parent->height + 1;
        cs[i].child.numChildren = -1;

        for (int j = 0; j < computeGranularity; ++j) {
          rng_spawn(parent->state.state, cs[i].child.state.state, static_cast<int>(i));
        }

        using scope = lf::scope<Context>;

        if (i + 1 == static_cast<std::size_t>(num_children)) {
          co_await scope::call(&cs[i].res, uts_fn{}, depth + 1, &cs[i].child);
        } else {
          co_await scope::fork(&cs[i].res, uts_fn{}, depth + 1, &cs[i].child);
        }
      }

      co_await lf::join();

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
void run(benchmark::State &state) {
  auto tree = static_cast<uts_tree>(state.range(0));

  setup_tree(tree);
  auto expected = expected_result(tree);

  Sch scheduler = [&] -> Sch {
    if constexpr (std::constructible_from<Sch, std::size_t>) {
      return Sch{static_cast<std::size_t>(state.range(1))};
    } else {
      return Sch{};
    }
  }();

  for (auto _ : state) {
    Node root;
    uts_initRoot(&root, type);
    lf::receiver recv = lf::schedule(scheduler, uts_fn{}, 0, &root);
    result r = std::move(recv).get();
    CHECK_RESULT(r, expected);
    benchmark::DoNotOptimize(r);
  }
}

} // namespace

// ---- Benchmark registrations ----

#define BENCH_ONE_MT(mode, tree_name, tree_id, ...)                                                          \
  BENCHMARK_TEMPLATE(run, __VA_ARGS__)                                                                       \
      ->Name(#mode "/libfork/uts/" tree_name "/" #__VA_ARGS__)                                               \
      ->Apply([](benchmark::Benchmark *b) -> void {                                                          \
        for (unsigned t = 1; t <= bench_max_threads; ++t) {                                                  \
          b->Args({tree_id, static_cast<std::int64_t>(t)});                                                  \
        }                                                                                                    \
      })                                                                                                     \
      ->UseRealTime();

#define BENCH_MT(...)                                                                                        \
  BENCH_ONE_MT(test, "T1_mini", uts_t1_mini, __VA_ARGS__)                                                    \
  BENCH_ONE_MT(test, "T3_mini", uts_t3_mini, __VA_ARGS__)                                                    \
  BENCH_ONE_MT(base, "T1", uts_t1, __VA_ARGS__)                                                              \
  BENCH_ONE_MT(base, "T3", uts_t3, __VA_ARGS__)                                                              \
  BENCH_ONE_MT(large, "T1L", uts_t1l, __VA_ARGS__)                                                           \
  BENCH_ONE_MT(large, "T3L", uts_t3l, __VA_ARGS__)

template <typename Stack, template <typename> typename Adaptor>
using real_context = lf::mono_context<Stack, Adaptor>;

template <typename Stack, template <typename> typename Adaptor>
using poly_context = lf::derived_poly_context<Stack, Adaptor>;

using lf::geometric_stack;

using mono_busy_pool = lf::mono_busy_pool<geometric_stack<>>;
using poly_busy_pool = lf::poly_busy_pool<geometric_stack<>>;

BENCH_MT(mono_busy_pool)
BENCH_MT(poly_busy_pool)
