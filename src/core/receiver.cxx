
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

/**
 * @brief Shared state between a scheduled task and its receiver handle.
 *
 * @tparam T          The return type of the scheduled coroutine.
 * @tparam Cancellable If true, the state owns a stop_source that can be used
 *                     to cancel the root task externally.
 *
 * Constructors forward arguments for in-place construction of the return value.
 * Internal access is gated behind a hidden friend: `get(key_t, receiver_state&)`.
 */
export template <typename T, bool Cancellable = false>
class receiver_state {
 public:
  struct empty {};

  /// Default construction — return value is default-initialised (or empty for void).
  constexpr receiver_state() = default;

  /// In-place construction of the return value from arbitrary args.
  template <typename... Args>
    requires (!std::is_void_v<T>) && std::constructible_from<T, Args...>
  constexpr explicit receiver_state(Args &&...args)
      : m_return_value(std::forward<Args>(args)...) {}

 private:
  template <typename U, bool C>
  friend class receiver;

  /**
   * @brief Internal accessor returned by `get(key_t, receiver_state&)`.
   *
   * Not reachable by name from outside this translation unit because view
   * is a private nested type. Callers use `auto` with the hidden friend.
   */
  struct view {
    receiver_state *p;

    constexpr void set_exception(std::exception_ptr e) noexcept {
      p->m_exception = std::move(e);
    }

    constexpr void notify_ready() noexcept {
      p->m_ready.test_and_set();
      p->m_ready.notify_one();
    }

    [[nodiscard]]
    constexpr auto return_value_address() noexcept -> T *
      requires (!std::is_void_v<T>)
    {
      return std::addressof(p->m_return_value);
    }

    [[nodiscard]]
    constexpr auto get_stop_token() noexcept -> stop_source::stop_token
      requires Cancellable
    {
      return p->m_stop.token();
    }
  };

  /**
   * @brief Hidden friend accessor for internal library use.
   *
   * Only callable via ADL when a `key_t` is available (i.e. by calling `key()`).
   * Returns a `view` proxy to manipulate the state's private members.
   */
  [[nodiscard]]
  friend constexpr auto get(key_t, receiver_state &self) noexcept -> view {
    return {&self};
  }

  [[no_unique_address]]
  std::conditional_t<std::is_void_v<T>, empty, T> m_return_value{};
  std::exception_ptr m_exception;
  std::atomic_flag m_ready;

  [[no_unique_address]]
  std::conditional_t<Cancellable, stop_source, empty> m_stop;
};

export template <typename T, bool Cancellable = false>
class receiver {

  using state_type = receiver_state<T, Cancellable>;

 public:
  constexpr receiver(key_t, std::shared_ptr<state_type> &&state) : m_state(std::move(state)) {}

  // Move only
  constexpr receiver(receiver &&) noexcept = default;
  constexpr receiver(const receiver &) = delete;
  constexpr auto operator=(receiver &&) noexcept -> receiver & = default;
  constexpr auto operator=(const receiver &) -> receiver & = delete;

  [[nodiscard]]
  constexpr auto valid() const noexcept -> bool {
    return m_state != nullptr;
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

  /**
   * @brief Returns a stop_token for this task's stop source.
   *
   * Only available when Cancellable=true.  The token can be used to request
   * cancellation of the scheduled task before or after it has started.
   */
  [[nodiscard]]
  constexpr auto token() noexcept -> stop_source::stop_token
    requires Cancellable
  {
    if (!valid()) {
      LF_THROW(broken_receiver_error{});
    }
    return get(key(), *m_state).get_stop_token();
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
