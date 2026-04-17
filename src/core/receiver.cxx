
module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.core:receiver;

import std;

import libfork.utils;

import :stop;

namespace lf {

export struct broken_receiver_error final : libfork_exception {
  [[nodiscard]]
  constexpr auto what() const noexcept -> const char * override {
    return "receiver is in invalid state";
  }
};

template <typename T>
struct receiver_state {

  struct empty {};

  [[no_unique_address]]
  std::conditional_t<std::is_void_v<T>, empty, T> m_return_value{};
  std::exception_ptr m_exception;
  std::atomic_flag m_ready;

  stop_source m_stop;
};

export template <typename T>
class receiver {

  using state_type = receiver_state<T>;

 public:
  constexpr receiver(key_t, std::shared_ptr<state_type> &&state) : m_state(std::move(state)) {}
  constexpr receiver(receiver &&) noexcept = default;
  constexpr auto operator=(receiver &&) noexcept -> receiver & = default;

  // Move only
  constexpr receiver(const receiver &) = delete;
  constexpr auto operator=(const receiver &) -> receiver & = delete;

  [[nodiscard]]
  constexpr auto valid() const noexcept -> bool {
    return m_state != nullptr;
  }

  /**
   * @brief Get a reference to the underlying stop_source.
   */
  [[nodiscard]]
  constexpr auto stop_source() noexcept -> stop_source & {
    if (!valid()) {
      LF_THROW(broken_receiver_error{});
    }
    return m_state->m_stop;
  }

  [[nodiscard]]
  constexpr auto ready() const -> bool {
    if (!valid()) {
      LF_THROW(broken_receiver_error{});
    }
    return m_state->m_ready.test();
  }

  constexpr void wait() const {
    if (!valid()) {
      LF_THROW(broken_receiver_error{});
    }
    m_state->m_ready.wait(false);
  }

  [[nodiscard]]
  constexpr auto get() && -> T {

    wait();

    // State will be cleaned up on unwind
    std::shared_ptr state = std::exchange(m_state, nullptr);

    LF_ASSUME(state != nullptr);

    if (state->m_exception) {
      std::rethrow_exception(state->m_exception);
    }

    if constexpr (!std::is_void_v<T>) {
      return std::move(state->m_return_value);
    }
  }

 private:
  std::shared_ptr<state_type> m_state;
};

} // namespace lf
