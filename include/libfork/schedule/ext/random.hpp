#ifndef CA0BE1EA_88CD_4E63_9D89_37395E859565
#define CA0BE1EA_88CD_4E63_9D89_37395E859565

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#include <utility>

/**
 * \file random.hpp
 *
 * @brief Pseudo random number generators (PRNG).
 *
 * This implementation has been adapted from ``http://prng.di.unimi.it/xoshiro256starstar.c``
 */

namespace lf {

namespace impl {

/**
 * @brief A tag type to disambiguated seeding from other operations.
 */
struct seed_t {};

template <typename T>
concept has_result_type = requires { typename std::remove_cvref_t<T>::result_type; };

template <typename G>
concept uniform_random_bit_generator_help =                           //
    std::uniform_random_bit_generator<G> &&                           //
    impl::has_result_type<G> &&                                       //
    std::same_as<std::invoke_result_t<G &>, typename G::result_type>; //

} // namespace impl

inline namespace ext {

inline constexpr impl::seed_t seed = {}; ///< A tag to disambiguate seeding from other operations.

/**
 * @brief `Like std::uniform_random_bit_generator`, but also requires a nested `result_type`.
 */
template <typename G>
concept uniform_random_bit_generator = impl::uniform_random_bit_generator_help<std::remove_cvref_t<G>>;

/**
 * @brief A \<random\> compatible implementation of the xoshiro256** 1.0 PRNG
 *
 * \rst
 *
 * From `the original <https://prng.di.unimi.it/>`_:
 *
 * This is xoshiro256** 1.0, one of our all-purpose, rock-solid generators. It has excellent
 * (sub-ns) speed, a state (256 bits) that is large enough for any parallel application, and it
 * passes all tests we are aware of.
 *
 * \endrst
 */
class xoshiro {
 public:
  using result_type = std::uint64_t; ///< Required by named requirement: UniformRandomBitGenerator

  /**
   * @brief Construct a new xoshiro with a fixed default-seed.
   */
  constexpr xoshiro() = default;

  /**
   * @brief Construct and seed the PRNG.
   *
   * @param my_seed The PRNG's seed, must not be everywhere zero.
   */
  explicit constexpr xoshiro(std::array<result_type, 4> const &my_seed) noexcept : m_state{my_seed} {}

  /**
   * @brief Construct and seed the PRNG from some other generator.
   */
  template <uniform_random_bit_generator PRNG>
    requires (!std::is_const_v<std::remove_reference_t<PRNG &&>>)
  constexpr xoshiro(impl::seed_t, PRNG &&dev) noexcept {
    for (std::uniform_int_distribution<result_type> dist{min(), max()}; auto &elem : m_state) {
      elem = dist(dev);
    }
  }

  /**
   * @brief Get the minimum value of the generator.
   *
   * @return The minimum value that ``xoshiro::operator()`` can return.
   */
  [[nodiscard]] static constexpr auto min() noexcept -> result_type {
    return std::numeric_limits<result_type>::lowest();
  }

  /**
   * @brief Get the maximum value of the generator.
   *
   * @return The maximum value that ``xoshiro::operator()`` can return.
   */
  [[nodiscard]] static constexpr auto max() noexcept -> result_type {
    return std::numeric_limits<result_type>::max();
  }

  /**
   * @brief Generate a random bit sequence and advance the state of the generator.
   *
   * @return A pseudo-random number.
   */
  [[nodiscard]] constexpr auto operator()() noexcept -> result_type {

    result_type const result = rotl(m_state[1] * 5, 7) * 9;
    result_type const temp = m_state[1] << 17; // NOLINT

    m_state[2] ^= m_state[0];
    m_state[3] ^= m_state[1];
    m_state[1] ^= m_state[2];
    m_state[0] ^= m_state[3];

    m_state[2] ^= temp;

    m_state[3] = rotl(m_state[3], 45); // NOLINT (magic-numbers)

    return result;
  }

  /**
   * @brief This is the jump function for the generator.
   *
   * It is equivalent to 2^128 calls to operator(); it can be used to generate 2^128 non-overlapping
   * sub-sequences for parallel computations.
   */
  constexpr auto jump() noexcept -> void {
    jump_impl({0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c}); // NOLINT
  }

  /**
   * @brief This is the long-jump function for the generator.
   *
   * It is equivalent to 2^192 calls to operator(); it can be used to generate 2^64 starting points,
   * from each of which jump() will generate 2^64 non-overlapping sub-sequences for parallel
   * distributed computations.
   */
  constexpr auto long_jump() noexcept -> void {
    jump_impl({0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635}); // NOLINT
  }

 private:
  /**
   * @brief The default seed for the PRNG.
   */
  std::array<result_type, 4> m_state = {
      0x8D0B73B52EA17D89,
      0x2AA426A407C2B04F,
      0xF513614E4798928A,
      0xA65E479EC5B49D41,
  };

  /**
   * @brief Utility function.
   */
  [[nodiscard]] static constexpr auto rotl(result_type const val, int const bits) noexcept -> result_type {
    return (val << bits) | (val >> (64 - bits)); // NOLINT
  }

  constexpr void jump_impl(std::array<result_type, 4> const &jump_array) noexcept {
    //
    std::array<result_type, 4> s = {0, 0, 0, 0}; // NOLINT

    for (result_type const jump : jump_array) {
      for (int bit = 0; bit < 64; ++bit) {  // NOLINT
        if (jump & result_type{1} << bit) { // NOLINT
          s[0] ^= m_state[0];
          s[1] ^= m_state[1];
          s[2] ^= m_state[2];
          s[3] ^= m_state[3];
        }
        std::invoke(*this);
      }
    }
    m_state = s;
  }
};

static_assert(uniform_random_bit_generator<xoshiro>);

} // namespace ext

} // namespace lf

#endif /* CA0BE1EA_88CD_4E63_9D89_37395E859565 */
