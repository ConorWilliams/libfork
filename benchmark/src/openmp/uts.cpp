#include <benchmark/benchmark.h>
#include <omp.h>

#include "common.hpp"
#include "macros.hpp"
#include "uts.hpp"

import std;

namespace {

auto uts_omp_impl(int depth, Node *parent) -> result {
  result r{.maxdepth = static_cast<counter_t>(depth), .size = counter_t{1}, .leaves = counter_t{0}};

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

      if (i + 1 == static_cast<std::size_t>(num_children)) {
        cs[i].res = uts_omp_impl(depth + 1, &cs[i].child);
      } else {
#pragma omp task shared(cs)
        cs[i].res = uts_omp_impl(depth + 1, &cs[i].child);
      }
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
void uts_run(benchmark::State &state) {
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

#define BENCH_MT()                                                                                           \
  UTS_BENCH_ONE_MT(uts_run, openmp, test, "T1_mini", uts_t1_mini)                                            \
  UTS_BENCH_ONE_MT(uts_run, openmp, test, "T3_mini", uts_t3_mini)                                            \
  UTS_BENCH_ONE_MT(uts_run, openmp, base, "T1", uts_t1)                                                      \
  UTS_BENCH_ONE_MT(uts_run, openmp, base, "T3", uts_t3)                                                      \
  UTS_BENCH_ONE_MT(uts_run, openmp, large, "T1L", uts_t1l)                                                   \
  UTS_BENCH_ONE_MT(uts_run, openmp, large, "T3L", uts_t3l)

BENCH_MT()
