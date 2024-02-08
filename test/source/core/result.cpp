// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <coroutine> // for suspend_always, coroutine_handle
#include <optional>  // for optional
#include <utility>   // for move
#include <vector>    // for vector

#include "libfork/core.hpp" // for discard_t, eventually, return_result

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

using namespace lf::impl;

namespace {

struct anything {
  template <typename T>
  operator T() const {
    return {};
  }
};

template <typename R, typename I>
struct coro {
  struct promise_type : return_result<R, I> {

    promise_type() : return_result<R, I>{anything{}} {}
    promise_type()
      requires std::is_void_v<R>
    = default;

    auto get_return_object() noexcept -> coro { return {}; }
    auto initial_suspend() noexcept -> std::suspend_always { return {}; }
    auto final_suspend() noexcept -> std::suspend_always { return {}; }
    void unhandled_exception() noexcept {}
  };
};

// root/non-root, R = void/non-void, T = void/non-void

// T = void

[[maybe_unused]] auto test1() -> coro<void, discard_t> { co_return; }

// T = non-reference

#define trivial(name, type)                                                                                  \
  [[maybe_unused]] auto name##_name() -> coro<int, type> {                                                   \
    int x = 23;                                                                                              \
    co_return 23;                                                                                            \
    co_return x;                                                                                             \
    co_return anything{};                                                                                    \
    co_return {};                                                                                            \
  }

trivial(discard_t, discard_t) trivial(int, int *) trivial(double, double *)
    trivial(int_o, std::optional<int> *) trivial(int_e, eventually<int> *)

#define vector(name, type)                                                                                   \
  [[maybe_unused]] auto vector_##name() -> coro<std::vector<int>, type> {                                    \
    std::vector<int> x;                                                                                      \
    co_return x;                                                                                             \
    co_return std::vector<int>{};                                                                            \
    co_return {};                                                                                            \
    co_return {x.begin(), x.end()};                                                                          \
    co_return {1, 2, 3};                                                                                     \
  }

        vector(void, discard_t) vector(vec, std::vector<int> *)
            vector(vec_o, std::optional<std::vector<int>> *) vector(vec_e, eventually<std::vector<int>> *)

    // ------------------------------------------- //

    int x = 23;

#define reference(name, type)                                                                                \
  [[maybe_unused]] auto reference_##name() -> coro<int &, type> { co_return x; }

#define rvalue_ref(name, type)                                                                               \
  [[maybe_unused]] auto rvalue_ref_##name() -> coro<int &&, type> {                                          \
    co_return 23;                                                                                            \
    co_return std::move(x);                                                                                  \
  }

reference(void, discard_t) reference(int, eventually<int &> *)

    rvalue_ref(void, discard_t) rvalue_ref(int, eventually<int &&> *)

} // namespace

// NOLINTEND