#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <coroutine>

#include "utility.hpp"

/**
 * @file task.hpp
 *
 * @brief The task class and associated utilities.
 *
 */

namespace lf {

/**
 * @brief ...
 *
 * @tparam T
 * @tparam Stack
 */
template <typename T, typename Stack>
class task;

/**
 * @brief ...
 *
 * @tparam T
 * @tparam Stack
 */
template <typename T, typename Stack>
class promise {
 public:
  //   task<T, Stack> get_return_object() noexcept {
  //     return task{std::coroutine_handle<promise_type>::from_promise(*this)};
  //   }

  //   std::suspend_always initial_suspend() const noexcept { return {}; }

  //   auto final_suspend() const noexcept { return {}; }

  //   // Pass regular awaitables straight through
  //   template <awaitable A>
  //   decltype(auto) await_transform(A&& a) {
  //     return std::forward<A>(a);
  //   }

  //   template <typename U>
  //   auto await_transform(detail::tag_fork<U> child) const noexcept {
  //     //
  //     struct awaitable : std::suspend_always {
  //       //
  //       std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> parent) {
  //         //
  //         _child.promise().template set_parent<T>(parent);

  //         // In-case *this (awaitable) is destructed by stealer after push
  //         std::coroutine_handle<> on_stack_handle = _child;

  //         LOG_DEBUG("Forking");

  //         detail::libfork::push({parent, std::addressof(parent.promise()._alpha)});

  //         return on_stack_handle;
  //       }

  //       Future<U> await_resume() const noexcept { return Future<U>{_child}; }

  //       std::coroutine_handle<typename Task<U>::promise_type> _child;
  //     };

  //     return awaitable{{}, std::move(child)};
  //   }

  //   auto await_transform(tag_sync) noexcept {
  //     struct awaitable {
  //       constexpr bool await_ready() const noexcept {
  //         if (std::uint64_t a = _task.promise()._alpha) {
  //           if (a == IMAX - _task.promise()._n.load(std::memory_order_acquire)) {
  //             LOG_DEBUG("sync() is ready");
  //             return true;
  //           } else {
  //             LOG_DEBUG("sync() not ready");
  //             return false;
  //           }
  //         } else {
  //           LOG_DEBUG("sync() ready (no steals)");
  //           return true;
  //         };
  //       }

  //       std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> task) noexcept
  //       {
  //         // Currently        n = IMAX - num_joined
  //         // If we perform    n <- n - (imax - num_stolen)
  //         // then             n = num_stolen - num_joined

  //         std::uint64_t a = task.promise()._alpha;
  //         std::uint64_t n = task.promise()._n.fetch_sub(IMAX - a, std::memory_order_acq_rel);

  //         if (n - (IMAX - a) == 0) {
  //           // We set n after all children had completed therefore we can resume
  //           // task
  //           LOG_DEBUG("sync() wins");
  //           return task;
  //         } else {
  //           // Someone else is responsible for running this task and we have run
  //           // out of work
  //           LOG_DEBUG("sync() looses");
  //           return std::noop_coroutine();
  //         }
  //       }

  //       constexpr void await_resume() const noexcept {
  //         // After a sync we reset a/n
  //         _task.promise()._alpha = 0;
  //         _task.promise()._n.store(IMAX, std::memory_order_release);
  //       }

  //       std::coroutine_handle<promise_type> _task;
  //     };

  //     // Check if num-joined == num-stolen
  //     return awaitable{std::coroutine_handle<promise_type>::from_promise(*this)};
  //   }

  //  private:
  //   std::coroutine_handle<> _parent;
  //   std::atomic_uint64_t* _parent_n;

  //   std::uint64_t _alpha = 0;

  //   alignas(hardware_destructive_interference_size) std::atomic_uint64_t _n = IMAX;

  //   static constexpr std::uint64_t IMAX = std::numeric_limits<std::uint64_t>::max();

  //   template <typename>
  //   friend class Task;  // Friend all tasks

  //   template <typename U>
  //   void set_parent(std::coroutine_handle<typename Task<U>::promise_type> parent) noexcept {
  //     _parent = parent;
  //     _parent_n = std::addressof(parent.promise()._n);
  //   }
};

}  // namespace lf