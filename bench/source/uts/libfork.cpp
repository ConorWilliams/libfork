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

#include <algorithm>
#include <iostream>

#include <benchmark/benchmark.h>

// #define LF_DEFAULT_LOGGING

#include <libfork.hpp>

#include "../util.hpp"
#include "config.hpp"
#include "external/uts.h"

namespace {

inline constexpr lf::async uts_alloc = [](auto uts, int depth, Node *parent) -> lf::task<result> {
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

      co_await lf::fork(cs[i].res, uts)(depth + 1, &cs[i].child);
    }

    co_await lf::join;

    for (auto &&elem : cs) {
      r.maxdepth = max(r.maxdepth, elem.res.maxdepth);
      r.size += elem.res.size;
      r.leaves += elem.res.leaves;
    }
  } else {
    r.leaves = 1;
  }
  co_return r;
};

// inline constexpr lf::async uts = [](auto uts, int depth, Node *parent) -> lf::task<result> {
//   //
//   int numChildren, childType;
//   counter_t parentHeight = parent->height;

//   result r(depth, 1, 0);

//   numChildren = uts_numChildren(parent);
//   childType = uts_childType(parent);

//   // record number of children in parent
//   parent->numChildren = numChildren;

//   // Recurse on the children
//   if (numChildren > 0) {

//     int i, j;

//     static_assert(lf::tag_of<decltype(uts)> != lf::tag::root);

//     // auto cs = lf::co_stack<result[]>::alloc(numChildren);

//     auto *cs = static_cast<result *>(co_await lf::stalloc{numChildren * sizeof(result)});

//     for (i = 0; i < numChildren; i++) {
//       Node child;

//       child.type = childType;
//       child.height = parentHeight + 1;
//       child.numChildren = -1; // not yet determined

//       for (j = 0; j < computeGranularity; j++) {
//         rng_spawn(parent->state.state, child.state.state, i);
//       }

//       result c;

//       co_await lf::fork(cs[i], uts)(depth + 1, &child);
//     }

//     co_await lf::join;

//     for (i = 0; i < numChildren; i++) {
//       if (cs[i].maxdepth > r.maxdepth) {
//         r.maxdepth = cs[i].maxdepth;
//       }
//       r.size += cs[i].size;
//       r.leaves += cs[i].leaves;
//     }

//     co_await lf::free{numChildren * sizeof(result)};

//   } else {
//     r.leaves = 1;
//   }

//   co_return r;
// };

// inline constexpr lf::async uts_shim = [](auto, int depth, Node *parent) -> lf::task<result> {
//   co_return co_await uts(depth, parent);
// };

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void uts_libfork_alloc(benchmark::State &state) {

  Sch sch(state.range(0));

  setup_uts();

  volatile int depth = 0;
  Node root;

  for (auto _ : state) {
    uts_initRoot(&root, type);
    volatile result r = sync_wait(sch, uts_alloc, depth, &root);
    // std::cout << "maxdepth: " << r.maxdepth << " size: " << r.size << " leaves: " << r.leaves << std::endl;
  }
}

// template <lf::scheduler Sch, lf::numa_strategy Strategy>
// void uts_libfork(benchmark::State &state) {

//   Sch sch(state.range(0));

//   setup_uts();

//   volatile int depth = 0;
//   Node root;

//   for (auto _ : state) {
//     uts_initRoot(&root, type);
//     volatile result res = sync_wait(sch, uts_shim, depth, &root);
//   }
// }

} // namespace

using namespace lf;

BENCHMARK(uts_libfork_alloc<lazy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
BENCHMARK(uts_libfork_alloc<lazy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();

BENCHMARK(uts_libfork_alloc<busy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
BENCHMARK(uts_libfork_alloc<busy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();

// BENCHMARK(uts_libfork<lazy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
// BENCHMARK(uts_libfork<lazy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();

// BENCHMARK(uts_libfork<busy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
// BENCHMARK(uts_libfork<busy_pool, numa_strategy::fan>)->DenseRange(1, num_threads())->UseRealTime();
