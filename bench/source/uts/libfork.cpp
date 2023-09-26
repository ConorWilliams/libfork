/*
 *         ---- The Unbalanced Tree Search (UTS) Benchmark ----
 *
 *  Copyright (c) 2010 See AUTHORS file for copyright holders
 *
 *  This file is part of the unbalanced tree search benchmark.  This
 *  project is licensed under the MIT Open Source license.  See the LICENSE
 *  file for copyright and licensing information.
 *
 *  UTS is a collaborative project between researchers at the University of
 *  Maryland, the University of North Carolina at Chapel Hill, and the Ohio
 *  State University.  See AUTHORS file for more information.
 *
 */

#include <iostream>

#include <benchmark/benchmark.h>

// #define LF_DEFAULT_LOGGING

#include <libfork.hpp>

#include "../util.hpp"
#include "config.hpp"
#include "external/uts.h"

namespace {

struct Result {
  counter_t maxdepth, size, leaves;
};

inline constexpr lf::async uts_alloc = [](auto uts, int depth, Node *parent) -> lf::task<Result> {
  //
  int numChildren, childType;
  counter_t parentHeight = parent->height;

  Result r(depth, 1, 0);

  numChildren = uts_numChildren(parent);
  childType = uts_childType(parent);

  // record number of children in parent
  parent->numChildren = numChildren;

  // Recurse on the children
  if (numChildren > 0) {

    int i, j;

    std::vector<Result> cs(numChildren);

    for (i = 0; i < numChildren; i++) {
      Node child;

      child.type = childType;
      child.height = parentHeight + 1;
      child.numChildren = -1; // not yet determined

      for (j = 0; j < computeGranularity; j++) {
        rng_spawn(parent->state.state, child.state.state, i);
      }

      Result c;

      co_await lf::fork(cs[i], uts)(depth + 1, &child);
    }

    co_await lf::join;

    for (i = 0; i < numChildren; i++) {
      if (cs[i].maxdepth > r.maxdepth) {
        r.maxdepth = cs[i].maxdepth;
      }
      r.size += cs[i].size;
      r.leaves += cs[i].leaves;
    }

  } else {
    r.leaves = 1;
  }

  co_return r;
};

inline constexpr lf::async uts = [](auto uts, int depth, Node *parent) -> lf::task<Result> {
  //
  int numChildren, childType;
  counter_t parentHeight = parent->height;

  Result r(depth, 1, 0);

  numChildren = uts_numChildren(parent);
  childType = uts_childType(parent);

  // record number of children in parent
  parent->numChildren = numChildren;

  // Recurse on the children
  if (numChildren > 0) {

    int i, j;

    static_assert(lf::tag_of<decltype(uts)> != lf::tag::root);

    // auto cs = lf::co_stack<Result[]>::alloc(numChildren);

    auto *cs = static_cast<Result *>(co_await lf::stalloc{numChildren * sizeof(Result)});

    for (i = 0; i < numChildren; i++) {
      Node child;

      child.type = childType;
      child.height = parentHeight + 1;
      child.numChildren = -1; // not yet determined

      for (j = 0; j < computeGranularity; j++) {
        rng_spawn(parent->state.state, child.state.state, i);
      }

      Result c;

      co_await lf::fork(cs[i], uts)(depth + 1, &child);
    }

    co_await lf::join;

    for (i = 0; i < numChildren; i++) {
      if (cs[i].maxdepth > r.maxdepth) {
        r.maxdepth = cs[i].maxdepth;
      }
      r.size += cs[i].size;
      r.leaves += cs[i].leaves;
    }

    co_await lf::free{numChildren * sizeof(Result)};

  } else {
    r.leaves = 1;
  }

  co_return r;
};

inline constexpr lf::async uts_shim = [](auto, int depth, Node *parent) -> lf::task<Result> {
  co_return co_await uts(depth, parent);
};

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void uts_libfork_alloc(benchmark::State &state) {

  Sch sch(state.range(0));

  setup_uts();

  volatile int depth = 0;
  Node root;

  for (auto _ : state) {
    uts_initRoot(&root, type);
    volatile Result res = sync_wait(sch, uts_alloc, depth, &root);
  }
}

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void uts_libfork(benchmark::State &state) {

  Sch sch(state.range(0));

  setup_uts();

  volatile int depth = 0;
  Node root;

  for (auto _ : state) {
    uts_initRoot(&root, type);
    volatile Result res = sync_wait(sch, uts_shim, depth, &root);
  }
}

} // namespace

using namespace lf;

BENCHMARK(uts_libfork_alloc<lazy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
BENCHMARK(uts_libfork_alloc<lazy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();

BENCHMARK(uts_libfork_alloc<busy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
BENCHMARK(uts_libfork_alloc<busy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();

BENCHMARK(uts_libfork<lazy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
BENCHMARK(uts_libfork<lazy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();

BENCHMARK(uts_libfork<busy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
BENCHMARK(uts_libfork<busy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();
