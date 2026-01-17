#include <algorithm>
#include <iostream>
#include <span>

#include <benchmark/benchmark.h>

// #define LF_DEFAULT_LOGGING

#include "concurrencpp/concurrencpp.h"

#include "../util.hpp"
#include "config.hpp"
#include "external/uts.h"

namespace {

using namespace concurrencpp;

struct pair2 {
  concurrencpp::result<::result> res;
  Node child;
};

auto uts(executor_tag, thread_pool_executor *tpe, int depth, Node *parent) -> concurrencpp::result<::result> {
  //
  ::result r(depth, 1, 0);

  int num_children = uts_numChildren(parent);
  int child_type = uts_childType(parent);

  parent->numChildren = num_children;

  if (num_children > 0) {

    std::vector<pair2> cs(num_children);

    for (int i = 0; i < num_children; i++) {

      cs[i].child.type = child_type;
      cs[i].child.height = parent->height + 1;
      cs[i].child.numChildren = -1; // not yet determined

      for (int j = 0; j < computeGranularity; j++) {
        rng_spawn(parent->state.state, cs[i].child.state.state, i);
      }

      cs[i].res = uts({}, tpe, depth + 1, &cs[i].child);
    }

    for (auto &&elem : cs) {

      auto res = co_await elem.res;

      r.maxdepth = max(r.maxdepth, res.maxdepth);
      r.size += res.size;
      r.leaves += res.leaves;
    }
  } else {
    r.leaves = 1;
  }
  co_return r;
};

void uts_ccpp(benchmark::State &state, int tree) {
  state.counters["green_threads"] = state.range(0);

  setup_tree(tree);

  volatile int depth = 0;
  Node root;

  ::result r;

  concurrencpp::runtime_options opt;
  opt.max_cpu_threads = state.range(0);
  concurrencpp::runtime runtime(opt);

  auto tpe = runtime.thread_pool_executor();

  for (auto _ : state) {
    uts_initRoot(&root, type);
    r = uts({}, tpe.get(), depth, &root).get();
  }

  if (r != result_tree(tree)) {
    std::cerr << "lf uts " << tree << " failed" << std::endl;
  }
}

} // namespace

MAKE_UTS_FOR(uts_ccpp);
