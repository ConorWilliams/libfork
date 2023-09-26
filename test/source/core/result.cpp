/*
 * Copyright (c) Conor Williams, Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <catch2/catch_test_macros.hpp>

#include "libfork/core/coroutine.hpp"
#include "libfork/core/result.hpp"

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

using namespace lf::impl;

namespace {

template <typename R, typename T>
struct coro {
  struct promise_type : promise_result<R, T> {
    // clang-format off
    promise_type() : promise_result<R, T>{nullptr} {}
    promise_type() requires is_void<R> = default;
    // clang-format on

    auto get_return_object() noexcept -> coro { return {}; }
    auto initial_suspend() noexcept -> stdx::suspend_always { return {}; }
    auto final_suspend() noexcept -> stdx::suspend_always { return {}; }
    void unhandled_exception() noexcept {}
  };
};

// root/non-root, R = void/non-void, T = void/non-void

struct anything {
  template <typename T>
  operator T() const {
    return {};
  }
};

// T = void

auto test1() -> coro<void, void> { co_return; }
auto test2() -> coro<root_result<void>, void> { co_return; }

// T = non-reference

#define trivial(name, type)                                                                                  \
  auto trivial_##name() -> coro<type, int> {                                                                 \
    int x = 23;                                                                                              \
    co_return 23;                                                                                            \
    co_return x;                                                                                             \
    co_return 34.;                                                                                           \
    co_return anything{};                                                                                    \
    co_return {};                                                                                            \
    co_return in_place{anything{}};                                                                          \
    co_return in_place{};                                                                                    \
    co_return in_place{78};                                                                                  \
  }

trivial(void, void);
trivial(int, int);
trivial(double, double);
trivial(root, root_result<int>);

#define vector(name, type)                                                                                   \
  auto vector_##name() -> coro<type, std::vector<int>> {                                                     \
    std::vector<int> x;                                                                                      \
    co_return x;                                                                                             \
    co_return std::vector<int>{};                                                                            \
    static std::vector<int> y;                                                                               \
    co_return y;                                                                                             \
    co_return {};                                                                                            \
    co_return {x.begin(), x.end()};                                                                          \
    co_return {1, 2, 3};                                                                                     \
    co_return in_place{};                                                                                    \
    co_return in_place{90};                                                                                  \
  }

vector(void, void);
vector(vec, std::vector<int>);
vector(root, root_result<std::vector<int>>);

// ------------------------------------------- //

#if !defined(_MSC_VER) || _MSC_VER >= 1936

struct I : lf::impl::immovable<I> {
  I() = default;
  I(int) {}
  I(int, int){};
};

auto test_immovable() -> coro<eventually<I>, I> {
  co_return {};
  co_return 1;
  co_return in_place{1, 2};
  co_return in_place{};
}

#endif

// ------------------------------------------- //

int x = 23;

#define reference(name, type)                                                                                \
  auto reference_##name() -> coro<type, int &> { co_return x; }

#define rreference(name, type)                                                                               \
  auto rreference_##name() -> coro<type, int &&> {                                                           \
    co_return 23;                                                                                            \
    co_return std::move(x);                                                                                  \
  }

reference(void, void);
reference(int, int);
reference(root, root_result<int>);

rreference(void, void);
rreference(int, int);
rreference(root, root_result<int>);

} // namespace

// NOLINTEND