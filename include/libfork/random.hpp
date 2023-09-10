#ifndef CA0BE1EA_88CD_4E63_9D89_37395E859565
#define CA0BE1EA_88CD_4E63_9D89_37395E859565

// The code in this file is adapted from the original implementation:
// http://prng.di.unimi.it/xoshiro256starstar.c

// Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

// To the extent possible under law, the author has dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide. This software is distributed without any warranty.

// See <http://creativecommons.org/publicdomain/zero/1.0/>.

#include <array>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

#include "libfork/macro.hpp"

/**
 * \file random.hpp
 *
 * @brief Pseudo random number generators (PRNG).
 */

namespace lf {

/**
 * @brief A utility for ensuring that two types, stripped of CV and ref qualifiers, are different.
 */
template <typename T, typename U>
concept different_from = !std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

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

  xoshiro() = default;

  xoshiro(xoshiro const &) = default;

  xoshiro(xoshiro &&) = default;

  auto operator=(xoshiro const &) -> xoshiro & = default;

  auto operator=(xoshiro &&) -> xoshiro & = default;

  ~xoshiro() = default;

  /**
   * @brief Construct and seed the PRNG.
   *
   * @param seed The PRNG's seed, must not be everywhere zero.
   */
  explicit constexpr xoshiro(std::array<result_type, 4> const &seed) : m_state{seed} {
    if (seed == std::array<result_type, 4>{0, 0, 0, 0}) {
      LF_ASSERT(false);
    }
  }

  /**
   * @brief Construct and seed the PRNG from some other generator.
   */
  template <different_from<xoshiro> PRNG>
    requires requires(PRNG &&device) {
      { std::invoke(device) } -> std::unsigned_integral;
    }
  constexpr explicit xoshiro(PRNG &&device)
      : m_state{random_bits(device), random_bits(device), random_bits(device), random_bits(device)} {
  } // NOLINT

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

  /**
   * @brief Utility function to upscale prng's result_type to xoshiro's result_type.
   */
  template <typename PRNG>
    requires requires(PRNG &&device) {
      { std::invoke(device) } -> std::unsigned_integral;
    }
  [[nodiscard]] static constexpr auto random_bits(PRNG &&device) -> result_type {
    //
    constexpr std::size_t chars_in_prng = sizeof(std::invoke_result_t<PRNG>);

    constexpr std::size_t chars_in_xoshiro = sizeof(result_type);

    static_assert(chars_in_xoshiro < chars_in_prng || chars_in_xoshiro % chars_in_prng == 0);

    result_type bits = std::invoke(device);

    for (std::size_t i = 1; i < chars_in_xoshiro / chars_in_prng; i++) {
      bits = (bits << CHAR_BIT * chars_in_prng) + std::invoke(device);
    }

    return bits;
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

} // namespace lf

#endif /* CA0BE1EA_88CD_4E63_9D89_37395E859565 */
