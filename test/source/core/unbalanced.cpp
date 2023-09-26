// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <memory>
#include <random>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

// #define LF_DEFAULT_LOGGING.

#include "libfork/core.hpp"
#include "libfork/core/macro.hpp"
#include "libfork/core/stack.hpp"

#include "libfork/schedule/busy_pool.hpp"
#include "libfork/schedule/lazy_pool.hpp"
#include "libfork/schedule/random.hpp"
#include "libfork/schedule/unit_pool.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

namespace {

template <typename T>
auto make_scheduler() -> T {
  if constexpr (std::constructible_from<T, std::size_t>) {
    return T{std::min(4U, std::thread::hardware_concurrency())};
  } else {
    return T{};
  }
}

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

  std::vector<int> vals;

  for (int i = 0; i < n; ++i) {
    vals.push_back(i);
  }

  std::shuffle(vals.begin(), vals.end(), rng);

  std::unique_ptr<tree> root = nullptr;
  std::uniform_real_distribution<double> dist{0, 1};

  for (auto val : vals) {
    if (dist(rng) < skew) {
      root = std::make_unique<tree>(tree{val, std::move(root), nullptr});
    } else {
      root = std::make_unique<tree>(tree{val, nullptr, std::move(root)});
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

  if (root.val == val) {
    return true;
  }

  bool left = false;
  bool right = false;

  if (root.left) {
    left = find(*root.left, val);
  }

  if (root.right) {
    right = find(*root.right, val);
  }

  return left || right;
}

} // namespace

TEST_CASE("tree checks", "[tree]") {

  int n = 100;

  std::unique_ptr root = build_tree(n, 0.5);

  REQUIRE(root);

  for (int i = 0; i < n; ++i) {
    REQUIRE(find(*root, i));
  }

  for (int i = n; i < 2 * n; ++i) {
    REQUIRE(!find(*root, i));
  }
}

// --------------------------------------------------------------- //

namespace {

inline constexpr async search = [](auto search, tree const &root, int val, auto *context) -> task<bool> {
  //
  if (root.val == val) {
    co_return true;
  }

  if (context && root.val % 10 == 0) {
    co_await context;

    if (search.context() != context) {
      LF_THROW(std::runtime_error("context not cleared"));
    }
  }

  bool left = false;
  bool right = false;

  if (root.left) {
    co_await lf::fork(left, search)(*root.left, val, context);
  }

  if (root.right) {
    co_await lf::fork(right, search)(*root.right, val, context);
  }

  co_await lf::join;

  co_return left || right;
};

}

TEMPLATE_TEST_CASE("tree search", "[tree][template]", unit_pool, debug_pool, busy_pool, lazy_pool) {

  int n = 1000;

  std::unique_ptr root = build_tree(n, 0.5);

  auto sch = make_scheduler<TestType>();

  // auto val

  context_of<TestType> *context = nullptr;

  for (int i = 0; i < n; ++i) {
    REQUIRE(sync_wait(sch, search, *root, i, context));
  }
}

TEMPLATE_TEST_CASE("tree bench", "[tree][template]", unit_pool, debug_pool, busy_pool, lazy_pool) {

  int n = 100;

  std::vector<int> vals;

  for (int i = 0; i < 2 * n; ++i) {
    vals.push_back(i);
  }

  std::array trees = {
      build_tree(n, 0.1),
      build_tree(n, 0.2),
      build_tree(n, 0.3),
      build_tree(n, 0.4),
      build_tree(n, 0.5),
  };

  for (auto &root : trees) {
    int count = 0;
    for (int val : vals) {
      count += find(*root, val);
    }
    REQUIRE(count == n);
  }

  auto sch = make_scheduler<TestType>();

  context_of<TestType> *context = nullptr;

  for (int i = 0; i < 10; ++i) {
    for (auto &root : trees) {
      int count = 0;
      for (int val : vals) {
        count += sync_wait(sch, search, *root, val, context);
      }
      REQUIRE(count == n);
    }
  }
}

// --------------------------------------------------------------- //

namespace {

inline constexpr async transfer = [](auto self, tree const &root, int val) -> task<bool> {
  co_return co_await search(root, val, self.context());
};

}

TEMPLATE_TEST_CASE("tree transfer", "[tree][template]", unit_pool, debug_pool, busy_pool, lazy_pool) {

  auto sch = make_scheduler<TestType>();

  int n = 100;

  std::vector<int> vals;

  for (int i = 0; i < n; ++i) {
    vals.push_back(i);
  }

  for (int i = 0; i < 10; ++i) {

    auto tree = build_tree(n, 0.5);

    for (int val : vals) {
      REQUIRE(sync_wait(sch, transfer, *build_tree(n, 0.5), val));
    }
  };
}

// NOLINTEND