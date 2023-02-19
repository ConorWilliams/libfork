// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <numeric>
#include <span>
#include <string>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

// #define NLOG
// #define NDEBUG

// NOLINTBEGIN No need to check the tests for style.

#define private public  // Yes I know this is bad but, we need to do bad things here!

#include "libfork/allocator.hpp"
#include "libfork/basic_task.hpp"
#include "libfork/inline.hpp"
#include "libfork/utility.hpp"

using namespace lf;

int global_count = 0;

template <typename T>
struct Tracked : std::allocator<T> {
  [[nodiscard]] constexpr T* allocate(std::size_t n) {
    DEBUG_TRACKER("ALLOCATING");
    global_count++;
    return std::allocator<T>::allocate(n);
  }

  Tracked() = default;

  template <typename U>
  Tracked(Tracked<U>) {}
};

template <typename T>
basic_task<T, inline_context, Tracked<T>> track(T x) {
  co_return x;  //
}

struct promise;

struct coroutine : std::coroutine_handle<promise> {
  using promise_type = struct promise;
};

struct promise : detail::allocator_mixin<Tracked<std::byte>> {
  coroutine get_return_object() { return {coroutine::from_promise(*this)}; }
  std::suspend_always initial_suspend() noexcept { return {}; }
  std::suspend_always final_suspend() noexcept { return {}; }
  void return_void() {}
  void unhandled_exception() {}
};

void trivial_coro() {
  coroutine h = []() -> coroutine { co_return; }();
  h.resume();
  h.destroy();
}

void manual() {
  inline_context context{};

  auto t = track(0);
  t->promise().m_context = &context;
  t->resume();
  t->destroy();
  t.release();
}

void handle() {
  inline_context context{};

  auto t = track(0);

  task_handle<inline_context> hand{t->promise()};

  t->promise().m_context = &context;
  t->resume();
  t->destroy();
  t.release();
}

void fut() {
  inline_context context{};

  basic_future<int, inline_context, Tracked<int>> t{track(0)};
  t->promise().m_context = &context;
  t->resume();
  t->destroy();
  t.release();
}

void both() {
  inline_context context{};

  auto t = track(0);

  task_handle<inline_context> hand{t->promise()};

  basic_future<int, inline_context, Tracked<int>> f{std::move(t)};
  f->promise().m_context = &context;
  f->resume();
  f->destroy();
  f.release();
}

void method() {
  inline_context context{};

  auto f = track(0).make_promise();
  f.future->promise().m_context = &context;
  f.future->resume();
  f.future->destroy();
  f.future.release();
}

TEST_CASE("HALO optimisation", "[basic_task][!mayfail][!nonportable]") {
  //
  REQUIRE(global_count == 0);

  trivial_coro();

  if (global_count != 0) {
    SKIP("HALO optimisation fails for trivial coroutines");
  }

  manual();

  if (global_count != 0) {
    FAIL("HALO optimisation fails at manual");
  }

  handle();

  if (global_count != 0) {
    FAIL("HALO optimisation fails at handle creation");
  }

  fut();

  if (global_count != 0) {
    FAIL("HALO optimisation fails at future creation");
  }

  both();

  if (global_count != 0) {
    FAIL("HALO optimisation fails at both");
  }

  method();

  if (global_count != 0) {
    FAIL("HALO optimisation fails at method");
  }
}

// NOLINTEND