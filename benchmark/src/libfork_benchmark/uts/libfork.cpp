#include <benchmark/benchmark.h>

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

      for (int i = 0; i < num_children; ++i) {
        cs[i].child.type = child_type;
        cs[i].child.height = parent->height + 1;
        cs[i].child.numChildren = -1;

        for (int j = 0; j < computeGranularity; ++j) {
          rng_spawn(parent->state.state, cs[i].child.state.state, i);
        }

        using scope = lf::scope<Context>;

        if (i + 1 == num_children) {
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
  auto tree_id = static_cast<int>(state.range(0));

  setup_tree(tree_id);
  auto expected = expected_result(tree_id);

  Sch scheduler = [&state] -> Sch {
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

template <typename Stack, template <typename> typename Adaptor>
using real_context = lf::mono_context<Stack, Adaptor>;

template <typename Stack, template <typename> typename Adaptor>
using poly_context = lf::derived_poly_context<Stack, Adaptor>;

using lf::adapt_deque;
using lf::adapt_vector;
using lf::inline_scheduler;

using lf::adaptor_stack;
using lf::geometric_stack;

// Single-threaded (inline_scheduler) variants

#define BENCH_ONE_ST(mode, tree_name, tree_id, ...)                                                          \
  BENCHMARK_TEMPLATE(run, __VA_ARGS__)                                                                       \
      ->Name(#mode "/libfork/uts/" tree_name "/" #__VA_ARGS__)                                               \
      ->Arg(tree_id)                                                                                         \
      ->UseRealTime();

#define BENCH_ST(...)                                                                                        \
  BENCH_ONE_ST(test, "T1", uts_t1, __VA_ARGS__)                                                              \
  BENCH_ONE_ST(base, "T1", uts_t1, __VA_ARGS__)                                                              \
  BENCH_ONE_ST(base, "T1L", uts_t1l, __VA_ARGS__)                                                            \
  BENCH_ONE_ST(base, "T3", uts_t3, __VA_ARGS__)                                                              \
  BENCH_ONE_ST(base, "T3L", uts_t3l, __VA_ARGS__)

BENCH_ST(inline_scheduler<real_context<adaptor_stack<>, adapt_vector>>)
BENCH_ST(inline_scheduler<poly_context<adaptor_stack<>, adapt_vector>>)

BENCH_ST(inline_scheduler<real_context<geometric_stack<>, adapt_vector>>)
BENCH_ST(inline_scheduler<poly_context<geometric_stack<>, adapt_vector>>)

BENCH_ST(inline_scheduler<real_context<geometric_stack<>, adapt_deque>>)
BENCH_ST(inline_scheduler<poly_context<geometric_stack<>, adapt_deque>>)

// Multi-threaded (busy_thread_pool) variants

#define BENCH_MAX_THR 8

#define BENCH_ONE_MT(mode, tree_name, tree_id, ...)                                                          \
  BENCHMARK_TEMPLATE(run, __VA_ARGS__)                                                                       \
      ->Name(#mode "/libfork/uts/" tree_name "/" #__VA_ARGS__)                                               \
      ->Apply([](benchmark::Benchmark *b) -> void {                                                          \
        for (unsigned t = 1; t <= BENCH_MAX_THR; ++t) {                                                      \
          b->Args({tree_id, static_cast<std::int64_t>(t)});                                                  \
        }                                                                                                    \
      })                                                                                                     \
      ->UseRealTime();

#define BENCH_MT(...)                                                                                        \
  BENCH_ONE_MT(test, "T1", uts_t1, __VA_ARGS__)                                                              \
  BENCH_ONE_MT(base, "T1", uts_t1, __VA_ARGS__)                                                              \
  BENCH_ONE_MT(base, "T1L", uts_t1l, __VA_ARGS__)                                                            \
  BENCH_ONE_MT(base, "T3", uts_t3, __VA_ARGS__)                                                              \
  BENCH_ONE_MT(base, "T3L", uts_t3l, __VA_ARGS__)

BENCH_MT(lf::busy_thread_pool<false, geometric_stack<>>)
BENCH_MT(lf::busy_thread_pool<true, geometric_stack<>>)
