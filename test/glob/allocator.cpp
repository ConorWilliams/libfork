// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <type_traits>

// NOLINTBEGIN No need to check the tests for style.

#include "libfork/schedule/busy_pool.hpp"
#include "libfork/task.hpp"

using namespace lf;

// //////////////////////////////////////////

template <typename T>
struct stateful : private std::allocator<T> {
  [[nodiscard]] constexpr auto allocate(std::size_t n) -> T* {
    m_count.fetch_add(1);
    return std::allocator<T>::allocate(n);
  }

  stateful(stateful const& other) : m_count(other.m_count.load()) {}

  using std::allocator<T>::deallocate;
  using typename std::allocator<T>::value_type;

  stateful() = default;  // Requires default constructible allocator.

  template <typename U>
  explicit stateful(stateful<U>) {}

  int count() const noexcept { return m_count.load(); }

 private:
  std::atomic<int> m_count = 0;
};

static_assert(std::allocator_traits<stateful<int>>::is_always_equal::value == false);

inline std::atomic<int> count = 0;

template <typename T>
struct stateless : std::allocator<T> {
  [[nodiscard]] constexpr auto allocate(std::size_t n) -> T* {
    count.fetch_add(1);
    return std::allocator<T>::allocate(n);
  }

  using std::allocator<T>::deallocate;
  using typename std::allocator<T>::value_type;

  stateless() = default;  // Requires default constructible allocator.

  template <typename U>
  explicit stateless(stateless<U>) {}
};

static_assert(std::allocator_traits<stateless<int>>::is_always_equal::value == true);

// //////////////////////////////////////////

static int fib(int n) {
  if (n <= 1) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

template <typename Alloc>
static basic_task<int, busy_pool::context, Alloc> fib_alloc(int n) {
  if (n < 2) {
    co_return n;
  }

  auto a = co_await fib_alloc<Alloc>(n - 1).fork();
  auto b = co_await fib_alloc<Alloc>(n - 2);

  co_await join();

  co_return *a + b;
}

template <typename StaticAlloc, typename Alloc = StaticAlloc>
static basic_task<int, busy_pool::context, Alloc> fib_alloc(std::allocator_arg_t, Alloc const& alloc, int n) {
  if (n < 2) {
    co_return n;
  }

  auto a = co_await fib_alloc<Alloc>(std::allocator_arg, alloc, n - 1).fork();
  auto b = co_await fib_alloc<Alloc>(std::allocator_arg, alloc, n - 2);

  co_await join();

  co_return *a + b;
}

/**
 * Cases
 *
 *  non-void + stateless+default_constructible, not passed in
 *  non-void + stateless+default_constructible, passed in
 *
 *  non-void + stateful, not passed in
 *  non-void + stateful, passed in
 *
 *  void + default new/delete
 *
 *  void + statefull
 *  void + stateless
 *
 */

TEST_CASE("Allocator non-void + stateless + not passed in", "[allocator]") {
  for (int i = 0; i < 20; ++i) {
    REQUIRE(busy_pool{}.schedule(fib_alloc<stateless<int>>(i)) == fib(i));
  }
}

TEST_CASE("Allocator non-void + stateless + passed in", "[allocator]") {
  for (int i = 0; i < 20; ++i) {
    REQUIRE(busy_pool{}.schedule(fib_alloc<stateless<int>>(std::allocator_arg, stateless<int>{}, i)) == fib(i));
  }
}

TEST_CASE("Allocator non-void + statefull + not passed in", "[allocator]") {
  for (int i = 0; i < 20; ++i) {
    REQUIRE(busy_pool{}.schedule(fib_alloc<stateful<int>>(i)) == fib(i));
  }
}

TEST_CASE("Allocator non-void + statefull + passed in", "[allocator]") {
  for (int i = 0; i < 20; ++i) {
    REQUIRE(busy_pool{}.schedule(fib_alloc<stateful<int>>(std::allocator_arg, stateful<int>{}, i)) == fib(i));
  }
}

TEST_CASE("Allocator void + default", "[allocator]") {
  for (int i = 0; i < 20; ++i) {
    REQUIRE(busy_pool{}.schedule(fib_alloc<void>(i)) == fib(i));
  }
}

TEST_CASE("Allocator void + stateless", "[allocator]") {
  for (int i = 0; i < 20; ++i) {
    REQUIRE(busy_pool{}.schedule(fib_alloc<void>(std::allocator_arg, stateless<int>{}, i)) == fib(i));
  }
}

TEST_CASE("Allocator void + statefull", "[allocator]") {
  for (int i = 0; i < 20; ++i) {
    REQUIRE(busy_pool{}.schedule(fib_alloc<void>(std::allocator_arg, stateful<int>{}, i)) == fib(i));
  }
}

// NOLINTEND