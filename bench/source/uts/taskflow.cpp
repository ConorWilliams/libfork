#include <algorithm>
#include <iostream>
#include <span>

#include <benchmark/benchmark.h>

// #define LF_DEFAULT_LOGGING

#include <libfork.hpp>

#include <taskflow/taskflow.hpp>

#include "../util.hpp"
#include "config.hpp"
#include "external/uts.h"

namespace {

auto uts(int depth, Node *parent, tf::Subflow &sbf) -> result {
  //
  result r(depth, 1, 0);

  int num_children = uts_numChildren(parent);
  int child_type = uts_childType(parent);

  parent->numChildren = num_children;

  if (num_children > 0) {

    std::vector<pair> cs(num_children);

    for (int i = 0; i < num_children; i++) {

      cs[i].child.type = child_type;
      cs[i].child.height = parent->height + 1;
      cs[i].child.numChildren = -1; // not yet determined

      for (int j = 0; j < computeGranularity; j++) {
        rng_spawn(parent->state.state, cs[i].child.state.state, i);
      }

      if (i + 1 == num_children) {
        sbf.corun([&cs, depth, i](tf::Subflow &sbf) {
          cs[i].res = uts(depth + 1, &cs[i].child, sbf);
        });
      } else {
        sbf.emplace([&cs, depth, i](tf::Subflow &sbf) {
          cs[i].res = uts(depth + 1, &cs[i].child, sbf);
        });
      }
    }

    sbf.join();

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

void uts_taskflow(benchmark::State &state, int tree) {

  std::size_t n = state.range(0);
  tf::Executor executor(n);
  tf::Taskflow taskflow;

  setup_tree(tree);

  volatile int depth = 0;
  Node root;

  result r;

  auto h = taskflow.emplace([&](tf::Subflow &sbf) {
    r = uts(depth, &root, sbf);
    return 4;
  });

  for (auto _ : state) {
    uts_initRoot(&root, type);

    executor.run(taskflow).wait();

    volatile result res = r;

    // std::cout << "maxdepth: " << r.maxdepth << " size: " << r.size << " leaves: " << r.leaves << std::endl;
  }
}

} // namespace

MAKE_UTS_FOR(uts_taskflow);
