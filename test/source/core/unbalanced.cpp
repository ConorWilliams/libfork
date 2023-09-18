// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <memory>
#include <random>
#include <ranges>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/core.hpp"

#include "libfork/macro.hpp"
#include "libfork/schedule/random.hpp"
#include "libfork/schedule/unit_pool.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

struct tree {
  int val;
  std::unique_ptr<tree> left;
  std::unique_ptr<tree> right;
};

/**
 * @brief Build an unbalanced tree containing all the numbers from 0 to n - 1.
 */
auto build_tree(int n, double skew) -> std::unique_ptr<tree> {

  REQUIRE(n > 0);
  REQUIRE(skew > 0);
  REQUIRE(skew < 1);

  if (n < 1) {
    return nullptr;
  }

  xoshiro rng{seed, std::random_device{}};

  auto iota = std::ranges::views::iota(0, n);

  std::vector<int> vals = {iota.begin(), iota.end()};

  std::shuffle(vals.begin(), vals.end(), rng);

  std::unique_ptr<tree> root = nullptr;
  std::uniform_real_distribution<double> dist{0, 1};

  for (auto val : vals) {
    if (dist(rng) < skew) {
      root = std::make_unique<tree>(val, std::move(root), nullptr);
    } else {
      root = std::make_unique<tree>(val, nullptr, std::move(root));
    }
  }

  auto untwist = [&](auto &self, tree &tree) -> void {
    if (dist(rng) < 0.5) {
      std::swap(tree.left, tree.right);
    }
    if (tree.left) {
      self(self, *tree.left);
    }
    if (tree.right) {
      self(self, *tree.right);
    }
  };

  untwist(untwist, *root);

  return root;
}

LF_NOINLINE auto find(tree const &root, int val) -> bool {
  return root.val == val || (root.left && find(*root.left, val)) || (root.right && find(*root.right, val));
}

TEST_CASE("tree checks", "[tree]") {

  int n = 100;

  std::unique_ptr root = build_tree(n, 0.5);

  REQUIRE(root);

  for (int val : std::ranges::views::iota(0, n)) {
    REQUIRE(find(*root, val));
  }

  for (int val : std::ranges::views::iota(n, 2 * n)) {
    REQUIRE(!find(*root, val));
  }
}

// --------------------------------------------------------------- //

inline constexpr async search = [](auto search, tree const &root, int val, auto *context) -> task<bool> {
  //
  if (root.val == val) {
    co_return true;
  }

  if (context && root.val % 10 == 0) {
    co_await context;
  }

  bool left = false;
  bool right = false;

  if (root.left) {
    co_await lf::fork(left, search)(*root.left, val, context);
  }

  if (root.right) {
    co_await lf::call(right, search)(*root.right, val, context);
  }

  co_await lf::join;

  co_return left || right;
};

TEMPLATE_TEST_CASE("tree search", "[tree][template]", unit_pool) {

  int n = 100;

  std::unique_ptr root = build_tree(n, 0.5);

  TestType sch{};

  // auto val

  context_of<TestType> *context = nullptr;

  for (int val : std::ranges::views::iota(0, n)) {
    REQUIRE(sync_wait(sch, search, *root, val, context));
  }
}

TEMPLATE_TEST_CASE("tree bench", "[tree][benchmark][template]", unit_pool) {

  TestType sch{};

  int n = 1000;

  auto iota = std::ranges::views::iota(0, 2 * n);
  std::vector<int> vals = {iota.begin(), iota.end()};

  std::array trees = {
      build_tree(n, 0.1), build_tree(n, 0.2), build_tree(n, 0.3), build_tree(n, 0.4), build_tree(n, 0.5),
  };

  for (auto &root : trees) {
    BENCHMARK("single-threaded") {
      int count = 0;
      for (int val : vals) {
        count += find(*root, val);
      }
      REQUIRE(count == n);
      return count;
    };
  }

  context_of<TestType> *context = nullptr;

  for (auto &root : trees) {
    BENCHMARK("async skew") {
      int count = 0;
      for (int val : vals) {
        count += sync_wait(sch, search, *root, val, context);
      }
      REQUIRE(count == n);
      return count;
    };
  }
}

// --------------------------------------------------------------- //

inline constexpr async transfer = [](auto self, tree const &root, int val) -> task<bool> {
  co_return co_await search(root, val, self.context());
};

TEMPLATE_TEST_CASE("tree transfer", "[tree][template]", unit_pool) {
  TestType sch{};

  int n = 100;

  auto iota = std::ranges::views::iota(0, n);

  std::vector<int> vals = {iota.begin(), iota.end()};

  BENCHMARK("transfer") {

    auto tree = build_tree(n, 0.5);

    for (int val : vals) {
      REQUIRE(sync_wait(sch, transfer, *build_tree(n, 0.5), val));
    }

    return tree;
  };
}