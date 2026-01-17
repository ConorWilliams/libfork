#include <algorithm>
#include <iostream>
#include <optional>
#include <span>

#include <benchmark/benchmark.h>

#include "tmc/all_headers.hpp"

#include "../util.hpp"
#include "config.hpp"
#include "external/uts.h"

namespace {

using namespace tmc;

auto uts(int depth, Node *parent) -> task<result> {
  //
  result r(depth, 1, 0);

  int num_children = uts_numChildren(parent);
  int child_type = uts_childType(parent);

  parent->numChildren = num_children;

  if (num_children > 0) {

    std::vector<Node> cs(num_children);
    std::vector<task<result>> tsk(num_children);

    for (int i = 0; i < num_children; i++) {

      cs[i].type = child_type;
      cs[i].height = parent->height + 1;
      cs[i].numChildren = -1; // not yet determined

      for (int j = 0; j < computeGranularity; j++) {
        rng_spawn(parent->state.state, cs[i].state.state, i);
      }

      tsk[i] = uts(depth + 1, &cs[i]);
    }

    auto results = co_await spawn_many(tsk.data(), num_children);

    for (auto &&res : results) {
      r.maxdepth = max(r.maxdepth, res.maxdepth);
      r.size += res.size;
      r.leaves += res.leaves;
    }
  } else {
    r.leaves = 1;
  }
  co_return r;
};

void uts_tmc(benchmark::State &state, int tree) {

  state.counters["green_threads"] = state.range(0);

  setup_tree(tree);

  volatile int depth = 0;
  Node root;

  result r;

  tmc::cpu_executor().set_thread_count(state.range(0)).init();

  for (auto _ : state) {
    uts_initRoot(&root, type);
    r = tmc::post_waitable(tmc::cpu_executor(), uts(depth, &root), 0).get();
  }

  tmc::cpu_executor().teardown();

  if (r != result_tree(tree)) {
    std::cerr << "lf uts " << tree << " failed" << std::endl;
  }
}

} // namespace

MAKE_UTS_FOR(uts_tmc);
