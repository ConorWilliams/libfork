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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.hpp"
#include "external/uts.h"

/***********************************************************
 * Recursive depth-first implementation                    *
 ***********************************************************/

namespace {

auto uts(int depth, Node *parent) -> result {

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

      cs[i].res = uts(depth + 1, &cs[i].child);
    }

    for (auto &&elem : cs) {
      r.maxdepth = max(r.maxdepth, elem.res.maxdepth);
      r.size += elem.res.size;
      r.leaves += elem.res.leaves;
    }
  } else {
    r.leaves = 1;
  }

  // std::cout << "maxdepth: " << r.maxdepth << " size: " << r.size << " leaves: " << r.leaves << std::endl;

  return r;
}

void uts_serial(benchmark::State &state, int tree) {

  Node root;

  setup_tree(tree);

  volatile int depth = 0;

  result r;

  for (auto _ : state) {
    uts_initRoot(&root, type);
    r = uts(depth, &root);
    // std::cout << "maxdepth: " << r.maxdepth << " size: " << r.size << " leaves: " << r.leaves << std::endl;
  }

  if (r != result_tree(tree)) {
    std::cerr << "serial uts " << tree << " failed" << std::endl;
  }
}

} // namespace

BENCHMARK_CAPTURE(uts_serial, "T1 Geo fixed ", 1)->UseRealTime();
BENCHMARK_CAPTURE(uts_serial, "T2 Geo cycle ", 2)->UseRealTime();
BENCHMARK_CAPTURE(uts_serial, "T3 Binomial  ", 3)->UseRealTime();
BENCHMARK_CAPTURE(uts_serial, "T4 Hybrid    ", 4)->UseRealTime();
BENCHMARK_CAPTURE(uts_serial, "T5 Geo linear", 5)->UseRealTime();

BENCHMARK_CAPTURE(uts_serial, "T1L Geo fixed ", 6)->UseRealTime();
BENCHMARK_CAPTURE(uts_serial, "T2L Geo cyclic", 7)->UseRealTime();
BENCHMARK_CAPTURE(uts_serial, "T3L Binomial  ", 8)->UseRealTime();
