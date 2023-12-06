#include <algorithm>
#include <iostream>
#include <span>

#include <benchmark/benchmark.h>

// #define LF_DEFAULT_LOGGING

#include <libfork.hpp>

#include <tbb/global_control.h>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>

#include "../util.hpp"
#include "config.hpp"
#include "external/uts.h"

namespace {

auto uts(int depth, Node *parent) -> result {
  //
  result r(depth, 1, 0);

  int num_children = uts_numChildren(parent);
  int child_type = uts_childType(parent);

  parent->numChildren = num_children;

  if (num_children > 0) {

    std::vector<pair> cs(num_children);

    tbb::task_group g;

    for (int i = 0; i < num_children; i++) {

      cs[i].child.type = child_type;
      cs[i].child.height = parent->height + 1;
      cs[i].child.numChildren = -1; // not yet determined

      for (int j = 0; j < computeGranularity; j++) {
        rng_spawn(parent->state.state, cs[i].child.state.state, i);
      }

      if (i + 1 == num_children) {
        cs[i].res = uts(depth + 1, &cs[i].child);
      } else {
        g.run([&cs, depth, i]() {
          cs[i].res = uts(depth + 1, &cs[i].child);
        });
      }
    }

    g.wait();

    for (auto &&elem : cs) {
      r.maxdepth = max(r.maxdepth, elem.res.maxdepth);
      r.size += elem.res.size;
      r.leaves += elem.res.leaves;
    }
  } else {
    r.leaves = 1;
  }
  return r;
};

void uts_tbb(benchmark::State &state, int tree) {

  // TBB uses (2MB) stacks by default
  tbb::global_control global_limit(tbb::global_control::thread_stack_size, 128 * 1024 * 1024);

  state.counters["green_threads"] = state.range(0);

  std::size_t n = state.range(0);
  tbb::task_arena arena(n);

  setup_tree(tree);

  volatile int depth = 0;
  Node root;

  result r;

  for (auto _ : state) {
    uts_initRoot(&root, type);

    r = arena.execute([&] {
      return uts(depth, &root);
    });

    // std::cout << "maxdepth: " << r.maxdepth << " size: " << r.size << " leaves: " << r.leaves << std::endl;
  }

  if (r != result_tree(tree)) {
    std::cerr << "tbb uts " << tree << " failed" << std::endl;
  }
}

} // namespace

MAKE_UTS_FOR(uts_tbb);
