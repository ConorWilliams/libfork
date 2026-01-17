#ifndef AD9A2908_3043_4CEC_9A2A_A57DE168DF19
#define AD9A2908_3043_4CEC_9A2A_A57DE168DF19

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for same_as
#include <type_traits> // for remove_cvref_t
#include <utility>     // for forward, as_const

#include "libfork/core/first_arg.hpp"         // for async_function_object, quasi_pointer, firs...
#include "libfork/core/impl/unique_frame.hpp" // for unique_frame
#include "libfork/core/impl/utility.hpp"      // for unqualified, immovable
#include "libfork/core/invocable.hpp"         // for async_result_t, return_address_for, async_...
#include "libfork/core/tag.hpp"               // for tag, modifier_for
#include "libfork/core/task.hpp"              // for returnable, task

/**
 * @file combinate.hpp
 *
 * @brief Utility for building a coroutine's first argument and invoking it.
 */

namespace lf::impl {

// ---------------------------- //

template <returnable R, return_address_for<R> I, tag Tag>
struct promise;

// -------------------------------------------------------- //

/**
 * @brief Awaitable in the context of an `lf::task` coroutine.
 *
 * This will be transformed by an `await_transform` and trigger a fork or call.
 *
 * This is really just a `task<T>` with a bit more static information.
 *
 * NOTE: This is created by `y_combinate`, the parent/semaphore needs to be set by the caller!
 */
template <returnable R, return_address_for<R> I, tag Tag, modifier_for<Tag> Mod>
struct [[nodiscard]] quasi_awaitable : immovable<quasi_awaitable<R, I, Tag, Mod>>, unique_frame {};

// ---------------------------- //

// TODO fixup the forwarding of types here.

/**
 * @brief Call an async function with a synthesized first argument.
 *
 * The first argument will contain a copy of the function hence, this is a fixed-point combinator.
 */
template <quasi_pointer I, tag Tag, modifier_for<Tag> Mod, async_function_object F>
  requires unqualified<I> && unqualified<F>
struct [[nodiscard("A bound function SHOULD be immediately invoked!")]] y_combinate {

  /**
   * @brief The return address.
   */
  [[no_unique_address]] I ret;
  /**
   * @brief The asynchronous function.
   */
  [[no_unique_address]] F fun;

  /**
   * @brief Invoke the coroutine, set's the return pointer.
   */
  template <typename... Args>
    requires async_tag_invocable<I, Tag, F, Args...>
  auto operator()(Args &&...args) && -> quasi_awaitable<async_result_t<F, Args...>, I, Tag, Mod> {

    task task = std::move(fun)(                                 //
        first_arg_t<I, Tag, F, Args &&...>(std::as_const(fun)), // Makes a copy of fun
        std::forward<Args>(args)...                             //
    );

    using promise = promise<async_result_t<F, Args...>, I, Tag>;

    if constexpr (!std::same_as<I, discard_t>) {
      // This upcast is safe as we know the real promise type.
      static_cast<promise *>(task.get())->set_return(std::move(ret));
    }

    return {{}, {std::move(task)}}; // This move just moves the base class.
  }
};

// ---------------------------- //

/**
 * @brief Build a combinator for `ret` and `fun`.
 */
template <tag Tag, modifier_for<Tag> Mod, quasi_pointer I, async_function_object F>
auto combinate(I &&ret, F &&fun) {

  using II = std::remove_cvref_t<I>;
  using FF = std::remove_cvref_t<F>;

  if constexpr (first_arg_specialization<F>) {
    // Must unwrap to prevent infinite type recursion.
    return y_combinate<II, Tag, Mod, typename FF::async_function>{
        std::forward<I>(ret),
        unwrap(std::forward<F>(fun)),
    };
  } else {
    return y_combinate<II, Tag, Mod, FF>{std::forward<I>(ret), std::forward<F>(fun)};
  }
}

} // namespace lf::impl

#endif /* AD9A2908_3043_4CEC_9A2A_A57DE168DF19 */
