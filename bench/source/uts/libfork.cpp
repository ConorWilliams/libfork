#include <algorithm>
#include <iostream>
#include <span>

#include <benchmark/benchmark.h>

// #define LF_DEFAULT_LOGGING

#include <libfork.hpp>

#include "../util.hpp"
#include "config.hpp"
#include "external/uts.h"

namespace {

constexpr auto uts_alloc = [](auto uts, int depth, Node *parent) LF_STATIC_CALL -> lf::task<result> {
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
        co_await lf::call(&cs[i].res, uts)(depth + 1, &cs[i].child);
      } else {
        co_await lf::fork(&cs[i].res, uts)(depth + 1, &cs[i].child);
      }
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

inline constexpr auto uts = [](auto uts, int depth, Node *parent) LF_STATIC_CALL -> lf::task<result> {
  //
  result r(depth, 1, 0);

  int num_children = uts_numChildren(parent);
  int child_type = uts_childType(parent);

  parent->numChildren = num_children;

  if (num_children > 0) {

    auto [cs] = co_await lf::co_new<pair>(num_children);

    for (int i = 0; i < num_children; i++) {

      cs[i].child.type = child_type;
      cs[i].child.height = parent->height + 1;
      cs[i].child.numChildren = -1; // not yet determined

      for (int j = 0; j < computeGranularity; j++) {
        rng_spawn(parent->state.state, cs[i].child.state.state, i);
      }

      if (i + 1 == num_children) {
        co_await lf::call(&cs[i].res, uts)(depth + 1, &cs[i].child);
      } else {
        co_await lf::fork(&cs[i].res, uts)(depth + 1, &cs[i].child);
      }
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

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void uts_libfork_alloc(benchmark::State &state, int tree) {

  state.counters["green_threads"] = state.range(0);

  Sch sch(state.range(0));

  setup_tree(tree);

  volatile int depth = 0;
  Node root;

  result r;

  for (auto _ : state) {
    uts_initRoot(&root, type);
    r = sync_wait(sch, uts_alloc, depth, &root);
  }

  if (r != result_tree(tree)) {
    std::cerr << "lf uts " << tree << " failed" << std::endl;
  }
}

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void uts_libfork(benchmark::State &state, int tree) {

  state.counters["green_threads"] = state.range(0);

  Sch sch(state.range(0));

  setup_tree(tree);

  volatile int depth = 0;
  Node root;

  result r;

  for (auto _ : state) {
    uts_initRoot(&root, type);
    r = sync_wait(sch, uts, depth, &root);
    // std::cout << "maxdepth: " << r.maxdepth << " size: " << r.size << " leaves: " << r.leaves << std::endl;
  }

  if (r != result_tree(tree)) {
    std::cerr << "lf uts " << tree << " failed" << std::endl;
  }
}

void uts_libfork_coalloc_lazy_seq(benchmark::State &state, int tree) {
  uts_libfork<lf::lazy_pool, lf::numa_strategy::seq>(state, tree);
}

void uts_libfork_coalloc_lazy_fan(benchmark::State &state, int tree) {
  uts_libfork<lf::lazy_pool, lf::numa_strategy::fan>(state, tree);
}

void uts_libfork_coalloc_busy_seq(benchmark::State &state, int tree) {
  uts_libfork<lf::busy_pool, lf::numa_strategy::seq>(state, tree);
}

void uts_libfork_coalloc_busy_fan(benchmark::State &state, int tree) {
  uts_libfork<lf::busy_pool, lf::numa_strategy::fan>(state, tree);
}

// Allocating

void uts_libfork_alloc_lazy_seq(benchmark::State &state, int tree) {
  uts_libfork_alloc<lf::lazy_pool, lf::numa_strategy::seq>(state, tree);
}

void uts_libfork_alloc_lazy_fan(benchmark::State &state, int tree) {
  uts_libfork_alloc<lf::lazy_pool, lf::numa_strategy::fan>(state, tree);
}

void uts_libfork_alloc_busy_seq(benchmark::State &state, int tree) {
  uts_libfork_alloc<lf::busy_pool, lf::numa_strategy::seq>(state, tree);
}

void uts_libfork_alloc_busy_fan(benchmark::State &state, int tree) {
  uts_libfork_alloc<lf::busy_pool, lf::numa_strategy::fan>(state, tree);
}

} // namespace

using namespace lf;

MAKE_UTS_FOR(uts_libfork_alloc_lazy_seq);
MAKE_UTS_FOR(uts_libfork_alloc_lazy_fan);
MAKE_UTS_FOR(uts_libfork_alloc_busy_seq);
MAKE_UTS_FOR(uts_libfork_alloc_busy_fan);

MAKE_UTS_FOR(uts_libfork_coalloc_lazy_seq);
MAKE_UTS_FOR(uts_libfork_coalloc_lazy_fan);
MAKE_UTS_FOR(uts_libfork_coalloc_busy_seq);
MAKE_UTS_FOR(uts_libfork_coalloc_busy_fan);
