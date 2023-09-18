#ifndef BC7496D2_E762_43A4_92A3_F2AD10690569
#define BC7496D2_E762_43A4_92A3_F2AD10690569

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <concepts>
#include <type_traits>
#include <utility>

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

namespace lf {

inline namespace ext {

/**
 * @brief A multi-producer, single-consumer intrusive list.
 *
 * This implementation is lock-free, allocates no memory and is optimized for weak memory models.
 */
template <typename T>
class intrusive_list : impl::immovable<intrusive_list<T>> {
 public:
  /**
   * @brief An intruded
   */
  class node : impl::immovable<node> {
   public:
    explicit constexpr node(T const &data) : m_data(data) {}

    constexpr auto get() noexcept -> T & { return m_data; }

   private:
    friend class intrusive_list;

    T m_data;
    node *m_next;
  };

  /**
   * @brief Push a new node, this can be called concurrently from any number of threads.
   */
  constexpr void push(node *new_node) noexcept {

    node *stale_head = m_head.load(std::memory_order_relaxed);

    for (;;) {
      non_null(new_node)->m_next = stale_head;

      if (m_head.compare_exchange_weak(stale_head, new_node, std::memory_order_release)) {
        return;
      }
    }
  }

  /**
   * @brief Pop all the nodes from the list and call `func` on each of them.
   *
   * Only the owner (thread) of the list can call this function. The nodes will be processed in FILO order.
   *
   * @return `false` if the list was empty, `true` otherwise.
   */
  template <typename F>
    requires std::is_nothrow_invocable_v<F, T &>
  constexpr auto consume(F &&func) noexcept -> bool {

    node *last = m_head.exchange(nullptr, std::memory_order_consume);

    for (node *walk = last; walk; walk = walk->m_next) {
      std::invoke(func, walk->m_data);
    }

    return last != nullptr;
  }

 private:
  std::atomic<node *> m_head = nullptr;
};

template <typename T>
using intrusive_node = typename intrusive_list<T>::node;

} // namespace ext

} // namespace lf

#endif /* BC7496D2_E762_43A4_92A3_F2AD10690569 */
