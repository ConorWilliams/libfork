export module libfork.core:stop;

import std;

import libfork.utils;

namespace lf {

/**
 * @brief Similar to a linked-list of std::stop_sorce but with an embedded stop_state.
 */
export class stop_source {
 public:
  /**
   * @brief Lightweight public handle to a stop_source chain.
   *
   * A stop_token is a non-owning pointer-sized wrapper around a stop_source.
   */
  class stop_token {
   public:
    /**
     * @brief Construct a null (non-cancellable) token.
     */
    constexpr stop_token() noexcept = default;

    /**
     * @brief Returns true if a stop source is associated (cancellation is possible).
     */
    [[nodiscard]]
    constexpr auto stop_possible() const noexcept -> bool {
      return m_src != nullptr;
    }

    /**
     * @brief Returns true if any stop source in the ancestor chain has been stopped.
     *
     * A null token always returns false.
     */
    [[nodiscard]]
    constexpr auto stop_requested() const noexcept -> bool {
      return deep_stop_requested(m_src);
    }

   private:
    friend class stop_source;

    explicit constexpr stop_token(stop_source const *src) noexcept : m_src(src) {}

    stop_source const *m_src = nullptr;
  };

  /**
   * @brief Construct a root stop source with no parent.
   */
  constexpr stop_source() noexcept = default;

  /**
   * @brief Construct a stop source chained onto the given parent token.
   */
  constexpr explicit stop_source(stop_token parent) noexcept : m_parent(parent.m_src) {}

  // Immovable
  constexpr stop_source(const stop_source &) noexcept = delete;
  constexpr stop_source(stop_source &&) noexcept = delete;
  constexpr auto operator=(const stop_source &) noexcept -> stop_source & = delete;
  constexpr auto operator=(stop_source &&) noexcept -> stop_source & = delete;

  /**
   * @brief Get a handle to this stop source.
   */
  constexpr auto token() const noexcept -> stop_token { return stop_token{this}; }

  /**
   * @brief Returns true if any stop source in the ancestor chain has been stopped.
   */
  [[nodiscard]]
  constexpr auto stop_requested() const noexcept -> bool {
    return deep_stop_requested(this);
  }

  /**
   * @brief Request that this stop source (and all its children) stop.
   */
  constexpr auto request_stop() noexcept -> void { m_stop.store(1, std::memory_order_release); }

  /**
   * @brief Same as `request_stop`, but returns true if this is the first time stop has been requested.
   */
  [[nodiscard("You can use request_stop() if you don't need the return value")]]
  constexpr auto race_request_stop() noexcept -> bool {
    return m_stop.exchange(1, std::memory_order_release) == 0;
  }

 private:
  /**
   * @brief Test if any stop request has been made in the current chain.
   *
   * Safe to call with a null pointer, in which case it returns false.
   */
  [[nodiscard]]
  friend constexpr auto deep_stop_requested(stop_source const *src) noexcept -> bool {
    for (stop_source const *ptr = src; ptr != nullptr; ptr = ptr->m_parent) {
      if (ptr->m_stop.load(std::memory_order_acquire) == 1) {
        return true;
      }
    }
    return false;
  }

  stop_source const *m_parent = nullptr;
  std::atomic<std::uint32_t> m_stop = 0;
};
} // namespace lf
