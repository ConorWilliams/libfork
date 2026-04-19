
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
 * This class is internal — users interact with it only via the exported
 * `root_state` wrapper (defined in the schedule partition) and the returned
 * `receiver` handle.
 *
 * The class embeds a 1 KiB aligned buffer that the root task's coroutine
 * frame is placement-new'd into (see the custom operator new/delete and
 * final_suspend awaiter in root.cxx).  Because the frame lives inside the
 * buffer, `receiver_state` must outlive the frame — arranged by holding a
 * type-erased `std::shared_ptr<void>` inside the root promise that is
 * hand-over-hand moved out of the frame before the frame is destroyed.
 */
template <typename T, bool Stoppable = false>
class receiver_state {
 public:
  /// Size of the embedded coroutine-frame buffer (bytes).
  static constexpr std::size_t buffer_size = 1024;

  struct empty {};

  /// Default construction — return value is default-initialised (or empty for void).
  constexpr receiver_state() = default;

  /// In-place construction of the return value from arbitrary args.
  template <typename... Args>
    requires (!std::is_void_v<T>) && std::constructible_from<T, Args...>
  constexpr explicit receiver_state(Args &&...args) : m_return_value(std::forward<Args>(args)...) {}

  /**
   * @brief Request that the associated task stop.
   *
   * Only available when Stoppable=true.  Safe to call before scheduling —
   * the root frame checks stop_requested() before executing the task body.
   */
  constexpr auto request_stop() noexcept -> void
    requires Stoppable
  {
    m_stop.request_stop();
  }

  /**
   * @brief Raw pointer to the embedded buffer.
   *
   * Used by the root task's promise_type::operator new to placement-construct
   * the coroutine frame inside this state's storage.
   */
  [[nodiscard]]
  auto buffer() noexcept -> void * {
    return m_buffer;
  }

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

    constexpr void set_exception(std::exception_ptr e) noexcept { p->m_exception = std::move(e); }

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
      requires Stoppable
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

  // Buffer first — it is the largest member and alignment-sensitive.
  alignas(std::max_align_t) std::byte m_buffer[buffer_size]{};

  [[no_unique_address]]
  std::conditional_t<std::is_void_v<T>, empty, T> m_return_value{};
  std::exception_ptr m_exception;
  std::atomic_flag m_ready;

  [[no_unique_address]]
  std::conditional_t<Stoppable, stop_source, empty> m_stop;
};

export template <typename T, bool Stoppable = false>
class receiver {

  using state_type = receiver_state<T, Stoppable>;

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
   * Only available when Stoppable=true.  The token can be used to observe
   * whether the associated task has been cancelled.
   */
  [[nodiscard]]
  constexpr auto token() noexcept -> stop_source::stop_token
    requires Stoppable
  {
    if (!valid()) {
      LF_THROW(broken_receiver_error{});
    }
    return get(key(), *m_state).get_stop_token();
  }

  /**
   * @brief Request that the associated task stop.
   *
   * Only available when Stoppable=true.  Thread-safe; may be called
   * concurrently with the task executing on worker threads.
   */
  constexpr auto request_stop() -> void
    requires Stoppable
  {
    if (!valid()) {
      LF_THROW(broken_receiver_error{});
    }
    m_state->m_stop.request_stop();
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
