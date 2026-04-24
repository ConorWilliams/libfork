#include <benchmark/benchmark.h>

#include "common.hpp"
#include "macros.hpp"
#include "uts.hpp"

import std;

namespace {

auto uts_traverse(int depth, Node *parent) -> result {

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

void uts_serial(benchmark::State &state, uts_tree tree) {
  setup_tree(tree);
  auto expect = expected_result(tree);

  for (auto _ : state) {
    Node root;
    uts_initRoot(&root, type);
    result r = uts_traverse(0, &root);

    if (r != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", r, expect));
      break;
    }

    benchmark::DoNotOptimize(r);
  }
}

} // namespace

UTS_BENCH_ALL(uts_serial, serial)

namespace {

auto uts_traverse_no_alloc(int depth, Node *parent) -> result {

  result r{.maxdepth = static_cast<counter_t>(depth), .size = counter_t{1}, .leaves = counter_t{0}};

  int num_children = uts_numChildren(parent);
  int child_type = uts_childType(parent);

  parent->numChildren = num_children;

  if (num_children > 0) {
    for (std::size_t i = 0; i < static_cast<std::size_t>(num_children); ++i) {

      pair cs;

      cs.child.type = child_type;
      cs.child.height = parent->height + 1;
      cs.child.numChildren = -1;

      for (int j = 0; j < computeGranularity; ++j) {
        rng_spawn(parent->state.state, cs.child.state.state, static_cast<int>(i));
      }

      cs.res = uts_traverse(depth + 1, &cs.child);

      r.maxdepth = std::max(r.maxdepth, cs.res.maxdepth);
      r.size += cs.res.size;
      r.leaves += cs.res.leaves;
    }
  } else {
    r.leaves = 1;
  }

  return r;
}

void uts_serial_no_alloc(benchmark::State &state, uts_tree tree) {
  setup_tree(tree);
  auto expect = expected_result(tree);

  for (auto _ : state) {
    Node root;
    uts_initRoot(&root, type);
    result r = uts_traverse_no_alloc(0, &root);

    if (r != expect) {
      state.SkipWithError(std::format("incorrect result: {} != {}", r, expect));
      break;
    }

    benchmark::DoNotOptimize(r);
  }
}

} // namespace

UTS_BENCH_ALL(uts_serial_no_alloc, serial / no_alloc)
