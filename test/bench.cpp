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

// #define NDEBUG
// #undef LIBFORK_LOG
// #define LIBFORK_LOGGING

#include "libfork/busy_pool.hpp"
#include "libfork/libfork.hpp"
#include "libfork/macro.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

class basic_context;

basic_context *glob;

class basic_context {
public:
  static constexpr std::size_t kilobyte = 1024 * 1024;

  using stack_type = virtual_stack<kilobyte>; // NOLINT

  static auto context() -> basic_context & { return *glob; }

  static void set(basic_context &ctx) { glob = &ctx; }

  static constexpr auto max_threads() noexcept -> std::size_t { return 1; }

  auto stack_top() -> stack_type::handle { return stack_type::handle{*stack}; }

  void stack_pop() {
    std::abort();
  }

  void stack_push(stack_type::handle) {
    std::abort();
  }

  auto task_pop() -> std::optional<task_handle> {
    if (tasks.empty()) {
      return std::nullopt;
    }
    return tasks.pop();
  }

  void task_push(task_handle task) {
    tasks.push(task);
  }

private:
  queue<task_handle> tasks;
  // std::stack<task_handle> tasks;

  typename stack_type::unique_ptr_t stack = stack_type::make_unique();
};

template <typename T>
using Task = task<T, basic_context>;

#define fork lf::fork
#define call lf::call
#define join lf::join

inline constexpr auto fib = fn([](auto fib, int n) -> Task<int> {
  //
  if (n < 2) {
    co_return n;
  }

  int a, b;

  co_await fork(a, fib)(n - 1);
  co_await call(b, fib)(n - 2);

  co_await join;

  co_return a + b;
});

int main() {
  volatile int x = 25;
  volatile int y;

  basic_context ctx;

  basic_context::set(ctx);

  for (int i = 0; i < 10; i++) {
    y = sync_wait([](auto handle) { handle(); }, fib, x);
  }
}
