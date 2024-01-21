#ifndef B29F7CE3_05ED_4A3D_A464_CBA0454226F0
#define B29F7CE3_05ED_4A3D_A464_CBA0454226F0

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for invocable
#include <functional>  // for identity, invoke
#include <iterator>    // for random_access_iterator, sized_sentinel_for
#include <optional>    // for nullopt, optional
#include <ranges>      // for begin, end, iterator_t, empty, random_acces...
#include <type_traits> // for decay_t

#include "libfork/algorithm/constraints.hpp" // for projected, indirectly_foldable, semigroup_t
#include "libfork/core/control_flow.hpp"     // for call, fork, join, rethrow_if_exception
#include "libfork/core/eventually.hpp"       // for eventually
#include "libfork/core/just.hpp"             // for just
#include "libfork/core/macro.hpp"            // for LF_ASSERT, LF_STATIC_CALL, LF_STATIC_CONST
#include "libfork/core/task.hpp"             // for task

/**
 * @file fold.hpp
 *
 * @brief A parallel adaptation of the `std::fold_[...]` family.
 */

namespace lf {

namespace impl {

namespace detail {

template <class Bop, std::random_access_iterator I, class Proj>
  requires indirectly_foldable<Bop, projected<I, Proj>>
using indirect_fold_acc_t = std::decay_t<semigroup_t<Bop &, std::iter_reference_t<projected<I, Proj>>>>;

template <std::random_access_iterator I,
          std::sized_sentinel_for<I> S,
          class Proj,
          indirectly_foldable<projected<I, Proj>> Bop>
struct fold_overload_impl {

  using acc = indirect_fold_acc_t<Bop, I, Proj>;
  using difference_t = std::iter_difference_t<I>;

  static constexpr bool async_bop = !std::invocable<Bop &, acc, std::iter_reference_t<projected<I, Proj>>>;

  /**
   * @brief Recursive implementation of `fold`, requires that `tail - head > 0`.
   */
  LF_STATIC_CALL auto
  operator()(auto fold, I head, S tail, difference_t n, Bop bop, Proj proj) LF_STATIC_CONST->lf::task<acc> {

    LF_ASSERT(n > 1);

    difference_t len = tail - head;

    LF_ASSERT(len > 0);

    if (len <= n) {

      auto init = acc(co_await just(proj)(*head)); // Require convertible to U

      for (++head; head != tail; ++head) {

        // Assignability to U.

        if constexpr (async_bop) {
          co_await call(&init, bop)(std::move(init), co_await just(proj)(*head));
          co_await rethrow_if_exception;
        } else {
          init = std::invoke(bop, std::move(init), co_await just(proj)(*head));
        }
      }

      co_return std::move(init);
    }

    auto mid = head + (len / 2);

    LF_ASSERT(mid - head > 0);
    LF_ASSERT(tail - mid > 0);

    eventually<acc> lhs;
    eventually<acc> rhs;

    co_await lf::fork(&lhs, fold)(head, mid, n, bop, proj);
    co_await lf::call(&rhs, fold)(mid, tail, n, bop, proj);

    co_await lf::join;

    co_return co_await just(std::move(bop))( //
        *std::move(lhs),                     //
        *std::move(rhs)                      //
    );                                       //
  }

  /**
   * @brief Recursive implementation of `fold` for `n = 1`, requires that `tail - head > 1`.
   *
   * You cannot parallelize a chunk smaller than or equal to size three, for example, `a + b + c`
   * requires `a + b` to be evaluated before adding the result to `c`.
   */
  LF_STATIC_CALL auto
  operator()(auto fold, I head, S tail, Bop bop, Proj proj) LF_STATIC_CONST->lf::task<acc> {

    difference_t len = tail - head;

    LF_ASSERT(len >= 0);

    switch (len) {
      case 0:
        LF_ASSERT(false && "Unreachable");
      case 1:
        co_return co_await lf::just(std::move(proj))(*head);
      default:
        auto mid = head + (len / 2);

        LF_ASSERT(mid - head > 0);
        LF_ASSERT(tail - mid > 0);

        eventually<acc> lhs;
        eventually<acc> rhs;

        co_await lf::fork(&lhs, fold)(head, mid, bop, proj);
        co_await lf::call(&rhs, fold)(mid, tail, bop, proj);

        co_await lf::join;

        co_return co_await just(std::move(bop))( //
            *std::move(lhs),                     //
            *std::move(rhs)                      //
        );                                       //
    }
  }
};

} // namespace detail

struct fold_overload {
  /**
   * @brief Recursive implementation of `fold` for `n = 1` case.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            class Proj = std::identity,
            indirectly_foldable<projected<I, Proj>> Bop>
  LF_STATIC_CALL auto operator()(auto /* unused */, I head, S tail, Bop bop, Proj proj = {})
      LF_STATIC_CONST->lf::task<std::optional<detail::indirect_fold_acc_t<Bop, I, Proj>>> {

    if (head == tail) {
      co_return std::nullopt;
    }

    co_return co_await lf::just(detail::fold_overload_impl<I, S, Proj, Bop>{})(
        std::move(head), std::move(tail), std::move(bop), std::move(proj) //
    );
  }

  /**
   * @brief Recursive implementation of `fold`.
   *
   * This will dispatch to the `n = 1` case if `n <= 3`.
   */
  template <std::random_access_iterator I,
            std::sized_sentinel_for<I> S,
            class Proj = std::identity,
            indirectly_foldable<projected<I, Proj>> Bop>
  LF_STATIC_CALL auto
  operator()(auto /* unused */, I head, S tail, std::iter_difference_t<I> n, Bop bop, Proj proj = {})
      LF_STATIC_CONST->lf::task<std::optional<detail::indirect_fold_acc_t<Bop, I, Proj>>> {

    if (head == tail) {
      co_return std::nullopt;
    }

    if (n == 1) {
      co_return co_await lf::just(detail::fold_overload_impl<I, S, Proj, Bop>{})(
          std::move(head), std::move(tail), std::move(bop), std::move(proj) //
      );
    }

    co_return co_await lf::just(detail::fold_overload_impl<I, S, Proj, Bop>{})(
        std::move(head), std::move(tail), n, std::move(bop), std::move(proj) //
    );
  }

  /**
   * @brief Range version.
   */
  template <std::ranges::random_access_range Range,
            class Proj = std::identity,
            indirectly_foldable<projected<std::ranges::iterator_t<Range>, Proj>> Bop>
    requires std::ranges::sized_range<Range>
  LF_STATIC_CALL auto operator()(auto /* unused */, Range &&range, Bop bop, Proj proj = {}) LF_STATIC_CONST
      ->lf::task<std::optional<detail::indirect_fold_acc_t<Bop, std::ranges::iterator_t<Range>, Proj>>> {

    if (std::ranges::empty(range)) {
      co_return std::nullopt;
    }

    using I = std::decay_t<decltype(std::ranges::begin(range))>;
    using S = std::decay_t<decltype(std::ranges::end(range))>;

    co_return co_await lf::just(detail::fold_overload_impl<I, S, Proj, Bop>{})(
        std::ranges::begin(range), std::ranges::end(range), std::move(bop), std::move(proj) //
    );
  }

  /**
   * @brief Range version.
   */
  template <std::ranges::random_access_range Range,
            class Proj = std::identity,
            indirectly_foldable<projected<std::ranges::iterator_t<Range>, Proj>> Bop>
    requires std::ranges::sized_range<Range>
  LF_STATIC_CALL auto operator()(auto /* unused */,
                                 Range &&range,
                                 std::ranges::range_difference_t<Range> n,
                                 Bop bop,
                                 Proj proj = {}) LF_STATIC_CONST
      ->lf::task<std::optional<detail::indirect_fold_acc_t<Bop, std::ranges::iterator_t<Range>, Proj>>> {

    if (std::ranges::empty(range)) {
      co_return std::nullopt;
    }

    using I = std::decay_t<decltype(std::ranges::begin(range))>;
    using S = std::decay_t<decltype(std::ranges::end(range))>;

    if (n == 1) {
      co_return co_await lf::just(detail::fold_overload_impl<I, S, Proj, Bop>{})(
          std::ranges::begin(range), std::ranges::end(range), std::move(bop), std::move(proj) //
      );
    }

    co_return co_await lf::just(detail::fold_overload_impl<I, S, Proj, Bop>{})(
        std::ranges::begin(range), std::ranges::end(range), n, std::move(bop), std::move(proj) //
    );
  }
};

} // namespace impl

/**
 * @brief Apply a binary operation to the elements of a range in parallel.
 */
inline constexpr impl::fold_overload fold = {};

} // namespace lf

#endif /* B29F7CE3_05ED_4A3D_A464_CBA0454226F0 */
