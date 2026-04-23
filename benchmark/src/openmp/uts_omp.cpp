#include <benchmark/benchmark.h>
#include <omp.h>

#include "common.hpp"
#include "uts.hpp"
#include "helpers.hpp"

import std;

namespace {

auto uts_omp_impl(int depth, Node *parent) -> result {
  result r{.maxdepth = static_cast<counter_t>(depth), .size = counter_t{1}, .leaves = counter_t{0}};

  int num_children = uts_numChildren(parent);
  int child_type = uts_childType(parent);

  parent->numChildren = num_children;

  if (num_children > 0) {
    // Cutoff: if depth is large, run serially to avoid task overhead
    if (depth > 10) {
      for (int i = 0; i < num_children; ++i) {
        Node child;
        child.type = child_type;
        child.height = parent->height + 1;
        child.numChildren = -1;
        for (int j = 0; j < computeGranularity; ++j) {
          rng_spawn(parent->state.state, child.state.state, i);
        }
        result res = uts_omp_impl(depth + 1, &child);
        r.maxdepth = std::max(r.maxdepth, res.maxdepth);
        r.size += res.size;
        r.leaves += res.leaves;
      }
      return r;
    }

    std::vector<pair> cs(static_cast<std::size_t>(num_children));

    for (std::size_t i = 0; i < static_cast<std::size_t>(num_children); ++i) {
      cs[i].child.type = child_type;
      cs[i].child.height = parent->height + 1;
      cs[i].child.numChildren = -1;

      for (int j = 0; j < computeGranularity; ++j) {
        rng_spawn(parent->state.state, cs[i].child.state.state, static_cast<int>(i));
      }

      #pragma omp task shared(cs)
      cs[i].res = uts_omp_impl(depth + 1, &cs[i].child);
    }

    #pragma omp taskwait

    for (auto &&elem : cs) {
      r.maxdepth = std::max(r.maxdepth, elem.res.maxdepth);
      r.size += elem.res.size;
      r.leaves += elem.res.leaves;
    }
  } else {
    r.leaves = 1;
  }

  return r;
}

template <typename = void>
void run(benchmark::State &state) {
  auto tree = static_cast<uts_tree>(state.range(0));
  int threads = static_cast<int>(state.range(1));

  setup_tree(tree);
  auto expect = expected_result(tree);

  state.counters["p"] = static_cast<double>(threads);
  state.SetComplexityN(static_cast<benchmark::IterationCount>(threads));

  for (auto _ : state) {
    Node root;
    uts_initRoot(&root, type);
    result r;

    omp_set_num_threads(threads);
    #pragma omp parallel
    {
      #pragma omp single
      {
        r = uts_omp_impl(0, &root);
      }
    }

    if (r != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", r, expect));
      break;
    }

    benchmark::DoNotOptimize(r);
  }
}

} // namespace

#define BENCH_MT(...)                                                                                        \
  OMP_UTS_BENCH_ONE_MT(run, test, "T1_mini", uts_t1_mini)                                                    \
  OMP_UTS_BENCH_ONE_MT(run, test, "T3_mini", uts_t3_mini)                                                    \
  OMP_UTS_BENCH_ONE_MT(run, base, "T1", uts_t1)                                                              \
  OMP_UTS_BENCH_ONE_MT(run, base, "T3", uts_t3)                                                              \
  OMP_UTS_BENCH_ONE_MT(run, large, "T1L", uts_t1l)                                                           \
  OMP_UTS_BENCH_ONE_MT(run, large, "T3L", uts_t3l)

BENCH_MT()
