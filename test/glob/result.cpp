// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <type_traits>
#include <vector>

#include "libfork/result.hpp"

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

static_assert(std::is_trivially_destructible_v<result<int>>);
static_assert(!std::is_trivially_destructible_v<result<std::vector<int>>>);

inline constexpr bool test_value = requires(result<int> r1, result<int> const r2, int a, int const& b) {
                                     { r1.return_value(a) } noexcept;
                                     { r1.return_value(b) } noexcept;

                                     { r1.get() } -> std::same_as<int&>;
                                     { r2.get() } -> std::same_as<int const&>;

                                     { std::move(r1).get() } -> std::same_as<int&&>;
                                     { std::move(r2).get() } -> std::same_as<int const&&>;
                                   };

static_assert(test_value);

inline constexpr bool test_ref = requires(result<int&> r1, result<int&> const r2, int a, int const b) {
                                   { r1.return_value(a) };

                                   { r1.get() } -> std::same_as<int&>;
                                   { r2.get() } -> std::same_as<int&>;

                                   { std::move(r1).get() } -> std::same_as<int&>;
                                   { std::move(r2).get() } -> std::same_as<int&>;
                                 };

static_assert(test_ref);

inline constexpr bool test_const_ref = requires(result<int const&> r1, result<int const&> const r2, int a, int const b) {
                                         { r1.return_value(a) };
                                         { r1.return_value(b) };

                                         { r1.get() } -> std::same_as<int const&>;
                                         { r2.get() } -> std::same_as<int const&>;

                                         { std::move(r1).get() } -> std::same_as<int const&>;
                                         { std::move(r2).get() } -> std::same_as<int const&>;
                                       };

static_assert(test_const_ref);

inline constexpr bool test_r_ref = requires(result<int&&> r1, result<int&&> const r2, int a, int const b) {
                                     { r1.return_value(std::move(a)) };

                                     { r1.get() } -> std::same_as<int&&>;
                                     { r2.get() } -> std::same_as<int&&>;

                                     { std::move(r1).get() } -> std::same_as<int&&>;
                                     { std::move(r2).get() } -> std::same_as<int&&>;
                                   };

static_assert(test_r_ref);

inline constexpr bool test_const_r_ref = requires(result<int const&&> r1, result<int const&&> const r2, int a, int const b) {
                                           { r1.return_value(std::move(a)) };
                                           { r1.return_value(std::move(b)) };

                                           { r1.get() } -> std::same_as<int const&&>;
                                           { r2.get() } -> std::same_as<int const&&>;

                                           { std::move(r1).get() } -> std::same_as<int const&&>;
                                           { std::move(r2).get() } -> std::same_as<int const&&>;
                                         };

static_assert(test_const_r_ref);

// NOLINTEND