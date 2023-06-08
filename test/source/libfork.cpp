// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <iostream>
#include <memory>
#include <new>
#include <semaphore>
#include <stack>
#include <type_traits>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "libfork/libfork.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

class basic_context : thread_local_ptr<basic_context> {
public:
  static constexpr std::size_t kilobyte = 1024 * 1024;

  using stack_type = virtual_stack<kilobyte>; // NOLINT

  static auto context() -> basic_context & { return get(); }

  using thread_local_ptr<basic_context>::set;

  static constexpr auto max_threads() noexcept -> std::size_t { return 1; }

  auto stack_top() -> stack_type::handle { return stack_type::handle{*stack}; }

  void stack_pop() {
    throw std::runtime_error("Should never be called");
  }

  void stack_push(stack_type::handle) {
    throw std::runtime_error("Should never be called");
  }

  auto task_pop() -> std::optional<task_handle> {
    if (tasks.empty()) {
      return std::nullopt;
    }
    auto task = tasks.top();
    tasks.pop();
    return task;
  }

  void task_push(task_handle task) {
    tasks.push(task);
  }

private:
  std::stack<task_handle> tasks;

  std::unique_ptr<stack_type> stack = std::make_unique<stack_type>();
};

template <typename T>
using Task = task<T, basic_context>;

// #define fork(f) lf::fork(LIFT(f))

#define fork lf::fork
#define call lf::call
#define join lf::join

inline constexpr auto fib = fn([](auto self, int n) -> Task<int> {
  if (n < 2) {
    co_return n;
  }

  // co_await fork[blob::b]();

  int a = 0, b = 0;

  std::cout << "n = " << n << "\n";

  co_await fork(a, self)(n - 1);
  co_await call(b, self)(n - 2);

  co_await join;

  co_return a + b;
});

struct poo {};

TEST_CASE("libfork", "[libfork]") {

  basic_context ctx;

  basic_context::set(ctx);

  // for (int i = 0; i < 20; ++i) {

  int i = 22;

  auto answer = sync_wait([](auto handle) { handle(); }, fib, i);

  std::cout << "fib(" << i << ") = " << answer << "\n";
}

// NOLINTEND