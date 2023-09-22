#ifndef E54125F4_034E_45CD_8DF4_7A71275A5308
#define E54125F4_034E_45CD_8DF4_7A71275A5308

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

#include "libfork/core/async.hpp"
#include "libfork/core/macro.hpp"
#include "libfork/core/meta.hpp"
#include "libfork/core/result.hpp"
#include "libfork/core/stack.hpp"
#include "libfork/core/utility.hpp"

/**
 * @file sync_wait.hpp
 *
 * @brief Utilities for synchronous execution of asynchronous functions.
 */

namespace lf {

inline namespace core {

/**
 * @brief A concept that schedulers must satisfy.
 */
template <typename Sch>
concept scheduler = requires (Sch &&sch, intruded_h<context_of<Sch>> *ext) { std::forward<Sch>(sch).schedule(ext); };

namespace detail {

template <typename Context, stateless F, typename... Args>
struct sync_wait_impl {

  template <typename R>
  using first_arg_t = impl::patched<Context, impl::basic_first_arg<R, tag::root, F>>;

  using dummy_packet = impl::packet<first_arg_t<void>, Args...>;
  using dummy_packet_value_type = value_of<std::invoke_result_t<F, dummy_packet, Args...>>;

  using real_packet = impl::packet<first_arg_t<impl::root_result<dummy_packet_value_type>>, Args...>;
  using real_packet_value_type = value_of<std::invoke_result_t<F, real_packet, Args...>>;

  static_assert(std::same_as<dummy_packet_value_type, real_packet_value_type>, "Value type changes!");
};

template <scheduler Sch, stateless F, typename... Args>
using result_t = typename sync_wait_impl<context_of<Sch>, F, Args...>::real_packet_value_type;

template <scheduler Sch, stateless F, typename... Args>
using packet_t = typename sync_wait_impl<context_of<Sch>, F, Args...>::real_packet;

} // namespace detail

/**
 * @brief The entry point for synchronous execution of asynchronous functions.
 *
 * This is a blocking call to an `lf::async` function. The function will be executed on `sch`.
 *
 * \rst
 *
 * .. warning::
 *
 *    This should never be called from within a coroutine.
 *
 * \endrst
 */
template <scheduler Sch, stateless F, class... Args>
auto sync_wait(Sch &&sch, [[maybe_unused]] async<F> fun, Args &&...args) noexcept -> detail::result_t<Sch, F, Args...> {

  impl::root_result<detail::result_t<Sch, F, Args...>> root_block;

  detail::packet_t<Sch, F, Args...> packet{{{root_block}}, std::forward<Args>(args)...};

  impl::frame_block *frame = std::move(packet).invoke();

  intruded_h<context_of<Sch>> link{std::bit_cast<submit_h<context_of<Sch>> *>(frame)};

  LF_LOG("Submitting root");

  std::forward<Sch>(sch).schedule(&link);

  LF_LOG("Acquire semaphore");

  root_block.semaphore.acquire();

  LF_LOG("Semaphore acquired");

  if constexpr (impl::non_void<detail::result_t<Sch, F, Args...>>) {
    return *std::move(root_block);
  }
}

} // namespace core

} // namespace lf

#endif /* E54125F4_034E_45CD_8DF4_7A71275A5308 */
