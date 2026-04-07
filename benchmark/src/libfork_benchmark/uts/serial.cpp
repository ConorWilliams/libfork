#include <benchmark/benchmark.h>

#include "libfork_benchmark/uts/uts.hpp"
#include "libfork_benchmark/common.hpp"

import std;

namespace {

auto uts_traverse(int depth, Node *parent) -> result {
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

      cs[i].res = uts_traverse(depth + 1, &cs[i].child);
    }

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

void uts_serial(benchmark::State &state) {
  auto tree = static_cast<uts_tree>(state.range(0));
  setup_tree(tree);
  auto expected = expected_result(tree);

  for (auto _ : state) {
    Node root;
    uts_initRoot(&root, type);
    result r = uts_traverse(0, &root);
    CHECK_RESULT(r, expected);
    benchmark::DoNotOptimize(r);
  }
}

} // namespace

BENCHMARK(uts_serial)->Name("test/serial/uts/T1")->Arg(uts_t1_test)->UseRealTime();
BENCHMARK(uts_serial)->Name("test/serial/uts/T3")->Arg(uts_t3_test)->UseRealTime();

BENCHMARK(uts_serial)->Name("base/serial/uts/T1")->Arg(uts_t1)->UseRealTime();
BENCHMARK(uts_serial)->Name("base/serial/uts/T3")->Arg(uts_t3)->UseRealTime();

BENCHMARK(uts_serial)->Name("large/serial/uts/T1L")->Arg(uts_t1l)->UseRealTime();
BENCHMARK(uts_serial)->Name("large/serial/uts/T3L")->Arg(uts_t3l)->UseRealTime();
