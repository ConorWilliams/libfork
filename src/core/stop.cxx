export module libfork.core:stop;

import std;

namespace lf {

struct stop_type {}; // Tag type

export [[nodiscard("You should immediately co_await this!")]]
constexpr auto child_stop_source() noexcept -> stop_type {
  return {};
}

/**
 * @brief An intrusively linked chain of stop sources.
 */
class stop_source {
 public:
  constexpr explicit stop_source(stop_source *parent) noexcept : m_parent(parent) {}
  constexpr stop_source() noexcept = default;

  // Immovable
  constexpr stop_source(const stop_source &) noexcept = delete;
  constexpr stop_source(stop_source &&) noexcept = delete;
  constexpr auto operator=(const stop_source &) noexcept -> stop_source & = delete;
  constexpr auto operator=(stop_source &&) noexcept -> stop_source & = delete;

  /**
   * @brief Request that this stop source (and all its children) stop.
   */
  constexpr auto request_stop() noexcept -> void { m_stop.store(1, std::memory_order_release); }

  /**
   * @brief Same as `request_stop`, but returns true if this is the first time stop has been requested.
   */
  constexpr auto race_request_stop() noexcept -> bool {
    return m_stop.exchange(1, std::memory_order_release) == 0;
  }

  /**
   * @brief Test if this stop source has been requested to stop.
   *
   * Note that this does not check parent stop sources, use `deep_stop_requested` for that.
   */
  [[nodiscard]]
  constexpr auto stop_requested() const noexcept -> bool {
    return m_stop.load(std::memory_order_acquire) == 1;
  }

  /**
   * @brief Test if any stop request has been made in the current chain.
   *
   * Safe to call with a null pointer, in which case it returns false.
   */
  [[nodiscard]]
  friend constexpr auto deep_stop_requested(stop_source const *src) noexcept -> bool {
    for (stop_source const *ptr = src; ptr != nullptr; ptr = ptr->m_parent) {
      if (ptr->stop_requested()) {
        return true;
      }
    }
    return false;
  }

 private:
  stop_source *m_parent = nullptr;
  std::atomic<std::uint32_t> m_stop = 0;
};

} // namespace lf
