// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_test_macros.hpp>
#include <coroutine>

#include "libfork/utility.hpp"

// NOLINTBEGIN No linting in tests

using namespace lf;

template <typename T>
constexpr bool all_good() {
  static_assert(awaitable<T>);
  static_assert(awaitable<T&>);
  static_assert(awaitable<T const>);
  static_assert(awaitable<T const&>);

  static_assert(awaitable<T, int>);
  static_assert(awaitable<T&, int>);
  static_assert(awaitable<T const, int>);
  static_assert(awaitable<T const&, int>);

  static_assert(!awaitable<T, std::string_view>);
  static_assert(!awaitable<T&, std::string_view>);
  static_assert(!awaitable<T const, std::string_view>);
  static_assert(!awaitable<T const&, std::string_view>);

  static_assert(std::is_same_v<await_result_t<T>, int>);
  static_assert(std::is_same_v<await_result_t<T&>, int>);
  static_assert(std::is_same_v<await_result_t<T const>, int>);
  static_assert(std::is_same_v<await_result_t<T const&>, int>);

  return true;
}

struct good_void {
  bool await_ready() const noexcept;
  void await_suspend(std::coroutine_handle<>) const noexcept;
  int await_resume() const noexcept;
};

struct good_bool {
  bool await_ready() const noexcept;
  void await_suspend(std::coroutine_handle<>) const noexcept;
  int await_resume() const noexcept;
};

struct good_coro {
  bool await_ready() const noexcept;
  std::coroutine_handle<good_void> await_suspend(std::coroutine_handle<>) const noexcept;
  int await_resume() const noexcept;
};

static_assert(all_good<good_void>());
static_assert(all_good<good_bool>());
static_assert(all_good<good_coro>());

// /////////////////////

struct member_co_await {
  good_void operator co_await() const noexcept;
};

struct operator_co_await {};

good_void operator co_await(operator_co_await) noexcept;

static_assert(all_good<member_co_await>());
static_assert(all_good<operator_co_await>());

// /////////////////////

struct both {
  good_void operator co_await() const noexcept;
};

good_void operator co_await(both) noexcept;

static_assert(!awaitable<both>);

// /////////////////////

struct r_value_await {
  good_void operator co_await() && noexcept;
};

static_assert(awaitable<r_value_await>);
static_assert(!awaitable<r_value_await&>);

// /////////////////////

struct double_void {
  bool await_ready() const noexcept;
  std::coroutine_handle<good_void> await_suspend(std::coroutine_handle<>) const noexcept;
  void await_resume() const noexcept;
};

static_assert(awaitable<double_void>);
static_assert(awaitable<double_void, void>);
static_assert(!awaitable<double_void, int>);

// NOLINTEND