#ifndef C0E5463D_72D1_43C1_9458_9797E2F9C033
#define C0E5463D_72D1_43C1_9458_9797E2F9C033

#include <bit>
#include <concepts>
#include <optional>

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

namespace lf {

inline namespace ext {

/**
 * @brief A fixed capacity, power-of-two FILO ring-buffer with customizable behavior on overflow/underflow.
 */
template <typename T, std::size_t N>
  requires std::constructible_from<T> && (std::has_single_bit(N))
class ring_buffer {

  struct return_nullopt {
    LF_STATIC_CALL constexpr auto operator()() LF_STATIC_CONST noexcept -> std::optional<T> { return {}; }
  };

  struct discard {
    LF_STATIC_CALL constexpr auto operator()(T const &) LF_STATIC_CONST noexcept -> bool { return false; }
  };

 public:
  /**
   * @brief Test whether the ring-buffer is empty.
   */
  [[nodiscard]] constexpr auto empty() const noexcept -> bool { return m_top == m_bottom; }

  /**
   * @brief Test whether the ring-buffer is full.
   */
  [[nodiscard]] constexpr auto full() const noexcept -> bool { return m_bottom - m_top == N; }

  /**
   * @brief Pushes a value to the ring-buffer.
   *
   * If the buffer is full then calls `when_full` with the value and returns the result, otherwise returns true.
   * By default, `when_full` is a no-op that returns false.
   */
  template <typename F = discard>
    requires std::is_invocable_r_v<bool, F, T const &>
  constexpr auto push(T const &val, F &&when_full = {}) noexcept(std::is_nothrow_invocable_v<F, T const &>) -> bool {
    if (full()) {
      return std::invoke(std::forward<F>(when_full), val);
    }
    store(m_bottom++, val);
    return true;
  }

  /**
   * @brief Pops (removes and returns) the last value pushed into the ring-buffer.
   *
   * If the buffer is empty calls `when_empty` and returns the result. By default, `when_empty` is a no-op that returns
   * a null `std::optional<T>`.
   */
  template <std::invocable F = return_nullopt>
    requires std::convertible_to<T &, std::invoke_result_t<F>>
  constexpr auto pop(F &&when_empty = {}) noexcept(std::is_nothrow_invocable_v<F>) -> std::invoke_result_t<F> {
    if (empty()) {
      return std::invoke(std::forward<F>(when_empty));
    }
    return load(--m_bottom);
  }

 private:
  auto store(std::size_t index, T const &val) noexcept -> void {
    m_buff[index & mask] = val; // NOLINT
  }

  [[nodiscard]] auto load(std::size_t index) noexcept -> T & {
    return m_buff[(index & mask)]; // NOLINT
  }

  static constexpr std::size_t mask = N - 1;

  std::size_t m_top = 0;
  std::size_t m_bottom = 0;
  std::array<T, N> m_buff;
};

} // namespace ext

} // namespace lf

#endif /* C0E5463D_72D1_43C1_9458_9797E2F9C033 */
