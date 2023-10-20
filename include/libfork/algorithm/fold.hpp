#ifndef B29F7CE3_05ED_4A3D_A464_CBA0454226F0
#define B29F7CE3_05ED_4A3D_A464_CBA0454226F0

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <iterator>
#include <numeric>
#include <optional>
#include <ranges>
#include <type_traits>

#include "libfork/core.hpp"

#include "libfork/algorithm/concepts.hpp"

/**
 * @file fold.hpp
 *
 * @brief A parallel adaptation of the `std::fold_[...]` family.
 */

namespace lf {

namespace impl {

namespace detail {

struct fold_overload_impl {
  /**
   * @brief Recursive implementation of `fold`, requires that `tail - head > 0`.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            class Proj = std::identity,
            indirectly_foldable<std::projected<I, Proj>> Bop>
  LF_STATIC_CALL auto operator()(auto fold, //
                                 I head,
                                 S tail,
                                 std::iter_difference_t<I> n,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->lf::task<indirect_result_t<Bop, I, I>> {

    LF_ASSERT(n > 3);

    std::iter_difference_t<I> len = tail - head;

    LF_ASSERT(len > 0);

    using acc_t = indirect_result_t<Bop, I, I>;

    if (len <= n) {
      acc_t acc(std::invoke(proj, *head));

      for (++head; head != tail; ++head) {
        if constexpr (async_fn<Bop>) {
          co_await call(acc, bop)(std::move(acc), std::invoke(proj, *head));
        } else {
          acc = std::invoke(bop, std::move(acc), std::invoke(proj, *head));
        }
      }
      co_return acc;
    }

    auto mid = head + (len / 2);

    eventually<acc_t> lhs;
    eventually<acc_t> rhs;

    co_await lf::fork(lhs, fold)(head, mid, n, bop, proj);
    co_await lf::call(rhs, fold)(mid, tail, n, bop, proj);

    co_await lf::join;

    if constexpr (async_fn<Bop>) {
      co_return co_await std::invoke(bop, std::move(*lhs), std::move(*rhs));
    } else {
      co_return std::invoke(bop, std::move(*lhs), std::move(*rhs));
    }
  }

  /**
   * @brief Recursive implementation of `fold` for `n <= 3`, requires that `tail - head > 1`.
   *
   * You cannot parallelize a chunk smaller than or equal to size three, for example, `a + b + c`
   * requires `a + b` to be evaluated before adding the result to `c`.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            class Proj = std::identity,
            indirectly_foldable<std::projected<I, Proj>> Bop>
  LF_STATIC_CALL auto operator()(auto fold, //
                                 I head,
                                 S tail,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST->lf::task<indirect_result_t<Bop, I, I>> {

    std::iter_difference_t<I> len = tail - head;

    LF_ASSERT(len > 0);

    using acc_t = indirect_result_t<Bop, I, I>;

    switch (len) {
      case 0:
      case 1:
        LF_ASSERT(false);
      case 2: {
        I lhs = head;
        I rhs = head + 1;

        if constexpr (async_fn<Bop>) {
          co_return co_await std::invoke(bop, std::invoke(proj, *lhs), std::invoke(proj, *rhs));
        } else {
          co_return std::invoke(bop, std::invoke(proj, *lhs), std::invoke(proj, *rhs));
        }
      }
      case 3: {
        auto mid = head + (len / 2);

        I l_lhs = head;
        I l_rhs = head + 1;

        I rhs = head + 2;

        if constexpr (async_fn<Bop>) {
          eventually<acc_t> lhs;
          co_await lf::call(lhs, bop)(std::invoke(proj, *l_lhs), std::invoke(proj, *l_rhs));
          co_return co_await std::invoke(bop, std::move(*lhs), std::invoke(proj, *rhs));
        } else {
          acc_t lhs = std::invoke(bop, std::invoke(proj, *l_lhs), std::invoke(proj, *l_rhs));
          co_return std::invoke(bop, std::move(lhs), std::invoke(proj, *rhs));
        }
      }
      default:
        auto mid = head + (len / 2);

        eventually<acc_t> lhs;
        eventually<acc_t> rhs;

        co_await lf::fork(lhs, fold)(head, mid, bop, proj);
        co_await lf::call(rhs, fold)(mid, tail, bop, proj);

        co_await lf::join;

        if constexpr (async_fn<Bop>) {
          co_return co_await std::invoke(bop, std::move(*lhs), std::move(*rhs));
        } else {
          co_return std::invoke(bop, std::move(*lhs), std::move(*rhs));
        }
    }
  }
};

inline constexpr async fold_impl = fold_overload_impl{};

} // namespace detail

struct fold_overload {
  /**
   * @brief Recursive implementation of `fold` for `n = 1` case.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            class Proj = std::identity,
            indirectly_foldable<std::projected<I, Proj>> Bop>
  LF_STATIC_CALL auto operator()(auto, I head, S tail, Bop bop, Proj proj = {})
      LF_STATIC_CONST->lf::task<std::optional<indirect_result_t<Bop, I, I>>> {

    auto len = tail - head;

    LF_ASSERT(len >= 0);

    switch (len) {
      case 0:
        co_return std::nullopt;
      case 1:
        co_return std::invoke(proj, *head);
      default:
        co_return co_await detail::fold_impl(head, tail, bop, proj);
    }
  }

  /**
   * @brief Recursive implementation of `fold`.
   *
   * This will dispatch to the `n = 1` case if `n <= 3`.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            class Proj = std::identity,
            indirectly_foldable<std::projected<I, Proj>> Bop>
  LF_STATIC_CALL auto operator()(auto, I head, S tail, std::iter_difference_t<I> n, Bop bop, Proj proj = {})
      LF_STATIC_CONST->lf::task<std::optional<indirect_result_t<Bop, I, I>>> {

    auto len = tail - head;

    LF_ASSERT(len >= 0);

    switch (len) {
      case 0:
        co_return std::nullopt;
      case 1:
        co_return std::invoke(proj, *head);
      default:
        LF_ASSERT(n >= 0);

        if (n <= 3) {
          co_return co_await detail::fold_impl(head, tail, bop, proj);
        } else {
          co_return co_await detail::fold_impl(head, tail, n, bop, proj);
        }
    }
  }

  /**
   * @brief Range version, dispatches to the iterator version.
   */
  template <std::ranges::random_access_range Range,
            class Proj = std::identity,
            indirectly_foldable<std::projected<std::ranges::iterator_t<Range>, Proj>> Bop>
    requires std::ranges::sized_range<Range>
  LF_STATIC_CALL auto
  operator()(auto fold, Range &&range, std::ranges::range_difference_t<Range> n, Bop bop, Proj proj = {})
      LF_STATIC_CONST->lf::task<std::optional<
          indirect_result_t<Bop, std::ranges::iterator_t<Range>, std::ranges::iterator_t<Range>>>> {
    co_return co_await fold(std::ranges::begin(range), std::ranges::end(range), n, bop, proj);
  }

  /**
   * @brief Range version, dispatches to the iterator version.
   */
  template <std::ranges::random_access_range Range,
            class Proj = std::identity,
            indirectly_foldable<std::projected<std::ranges::iterator_t<Range>, Proj>> Bop>
    requires std::ranges::sized_range<Range>
  LF_STATIC_CALL auto operator()(auto fold, Range &&range, Bop bop, Proj proj = {}) LF_STATIC_CONST->lf::task<
      std::optional<indirect_result_t<Bop, std::ranges::iterator_t<Range>, std::ranges::iterator_t<Range>>>> {
    co_return co_await fold(std::ranges::begin(range), std::ranges::end(range), bop, proj);
  }
};

} // namespace impl

/**
 * @brief Apply a binary operation to the elements of a range in parallel.
 */
inline constexpr async fold = impl::fold_overload{};

} // namespace lf

#endif /* B29F7CE3_05ED_4A3D_A464_CBA0454226F0 */
