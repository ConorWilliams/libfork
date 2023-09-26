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

struct Result {
  counter_t maxdepth, size, leaves;
};

auto parTreeSearch(int depth, Node *parent) -> Result {
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

    for (i = 0; i < numChildren; i++) {
      Node child;
      child.type = childType;
      child.height = parentHeight + 1;
      child.numChildren = -1; // not yet determined

      for (j = 0; j < computeGranularity; j++) {
        rng_spawn(parent->state.state, child.state.state, i);
      }

      Result c = parTreeSearch(depth + 1, &child);

      if (c.maxdepth > r.maxdepth) {
        r.maxdepth = c.maxdepth;
      }

      r.size += c.size;
      r.leaves += c.leaves;
    }
  } else {
    r.leaves = 1;
  }

  return r;
}

void uts_serial(benchmark::State &state) {

  Node root;

  setup_uts();

  volatile int depth = 0;

  for (auto _ : state) {
    uts_initRoot(&root, type);
    volatile Result r = parTreeSearch(depth, &root);
  }
}

BENCHMARK(uts_serial)->UseRealTime();