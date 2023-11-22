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

#include "libfork/core/macro.hpp"

#include "libfork/core/impl/utility.hpp"

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
   * @brief An intruded node in the list.
   */
  class node : impl::immovable<node> {
   public:
    /**
     * @brief Construct a node storing a copy of `data`.
     */
    explicit constexpr node(T const &data) : m_data(data) {}

    /**
     * @brief Access the value stored in a node of the list.
     */
    friend constexpr auto unwrap(node *ptr) noexcept -> T & { return non_null(ptr)->m_data; }

    /**
     * @brief Call `func` on each unwrapped node linked in the list.
     *
     * This is a noop if `root` is `nullptr`.
     */
    template <std::invocable<T &> F>
    friend constexpr void for_each_elem(node *root, F &&func) noexcept(std::is_nothrow_invocable_v<F, T &>) {
      while (root) {
        // Have to be very careful here, we can't deference `root` after
        // we've called `func` as `func` could destroy the node so, we have
        // to cache the next pointer before the function call.
        auto next = root->m_next;
        std::invoke(func, root->m_data);
        root = next;
      }
    }

   private:
    friend class intrusive_list;

    [[no_unique_address]] T m_data;
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
   * @brief Pop all the nodes from the list and return a pointer to the root (`nullptr` if empty).
   *
   * Only the owner (thread) of the list can call this function, this will reverse the direction of the list
   * such that `for_each_elem` will operate if FIFO order.
   */
  constexpr auto try_pop_all() noexcept -> node * {

    node *last = m_head.exchange(nullptr, std::memory_order_consume);
    node *first = nullptr;

    while (last) {
      node *tmp = last;
      last = last->m_next;
      tmp->m_next = first;
      first = tmp;
    }

    return first;
  }

 private:
  std::atomic<node *> m_head = nullptr;
};

/**
 * @brief A type alias for the node type of an intrusive list.
 */
template <typename T>
using intruded_list = typename intrusive_list<T>::node *;

} // namespace ext

} // namespace lf

#endif /* BC7496D2_E762_43A4_92A3_F2AD10690569 */
