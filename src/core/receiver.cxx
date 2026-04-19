
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
 */
template <typename T, bool Stoppable = false>
struct receiver_state {

  struct empty_1 {};
  struct empty_2 {};

  alignas(k_new_align) std::array<std::byte, 1024> buffer{};

  [[no_unique_address]]
  std::conditional_t<std::is_void_v<T>, empty_1, T> return_value{};

  std::exception_ptr exception;
  std::atomic_flag ready;

  [[no_unique_address]]
  std::conditional_t<Stoppable, stop_source, empty_2> stop;

  constexpr receiver_state() = default;

  template <typename... Args>
    requires (sizeof...(Args) > 0) && std::constructible_from<T, Args...>
  constexpr explicit(sizeof...(Args) == 1) receiver_state(Args &&...args)
      : return_value(std::forward<Args>(args)...) {}
};

/// Convenience alias — used throughout the core partitions.
template <typename T, bool Stoppable = false>
using state_handle = std::shared_ptr<receiver_state<T, Stoppable>>;

/**
 * @brief Lightweight move-only handle owning a pre-allocated root task state.
 *
 * Construction allocates a `receiver_state<T, Stoppable>` which embeds a
 * 1 KiB aligned buffer; the root coroutine frame is placement-constructed
 * into that buffer by `schedule`.
 *
 * Constructors mirror `make_shared` / `allocate_shared`:
 *
 *   root_state<T> s;                               // default-init return value
 *   root_state<T> s{v1, v2};                       // in-place init: T{v1, v2}
 *   root_state<T> s{allocator_arg, alloc};         // default-init, custom allocator
 *   root_state<T> s{allocator_arg, alloc, v1, v2}; // in-place init + custom allocator
 */
export template <typename T, bool Stoppable = false>
class root_state {
  using state_type = receiver_state<T, Stoppable>;

 public:
  /// Default: value-initialise via `std::make_shared`.
  constexpr root_state() : m_ptr(std::make_shared<state_type>()) {}

  /// Value-init from args: forwards `args` to `receiver_state`'s constructor
  /// (in-place construction of the return value) via `std::make_shared`.
  template <typename... Args>
    requires (sizeof...(Args) > 0) && std::constructible_from<state_type, Args...>
  constexpr explicit(sizeof...(Args) == 1) root_state(Args &&...args)
      : m_ptr(std::make_shared<state_type>(std::forward<Args>(args)...)) {}

  /// Allocator-aware, default return value: allocate via `std::allocate_shared`.
  template <simple_allocator Alloc>
  constexpr root_state(std::allocator_arg_t, Alloc const &alloc)
      : m_ptr(std::allocate_shared<state_type>(alloc)) {}

  /// Allocator-aware with value-init args.
  template <simple_allocator Alloc, typename... Args>
    requires std::constructible_from<state_type, Args...>
  constexpr root_state(std::allocator_arg_t, Alloc const &alloc, Args &&...args)
      : m_ptr(std::allocate_shared<state_type>(alloc, std::forward<Args>(args)...)) {}

  // Move-only.
  constexpr root_state(root_state &&) noexcept = default;
  constexpr auto operator=(root_state &&) noexcept -> root_state & = default;
  constexpr root_state(root_state const &) = delete;
  constexpr auto operator=(root_state const &) -> root_state & = delete;

 private:
  [[nodiscard]]
  friend constexpr auto get(key_t, root_state &&self) noexcept -> state_handle<T, Stoppable> {
    return std::move(self.m_ptr);
  }

  state_handle<T, Stoppable> m_ptr;
};

export template <typename T, bool Stoppable = false>
class receiver {

  using state_type = receiver_state<T, Stoppable>;

 public:
  constexpr receiver(key_t, state_handle<T, Stoppable> state) noexcept : m_state(std::move(state)) {}

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
    return m_state->ready.test();
  }

  constexpr void wait() const {
    if (!valid()) {
      LF_THROW(broken_receiver_error{});
    }
    m_state->ready.wait(false);
  }

  /**
   * @brief Returns a stop_token for this task's stop source.
   *
   * Only available when Stoppable=true.
   */
  [[nodiscard]]
  constexpr auto token() const -> stop_source::stop_token
    requires Stoppable
  {
    if (!valid()) {
      LF_THROW(broken_receiver_error{});
    }
    return m_state->stop.token();
  }

  /**
   * @brief Request that the associated task stop.
   *
   * Only available when Stoppable=true. Thread-safe.
   */
  constexpr auto request_stop() -> void
    requires Stoppable
  {
    if (!valid()) {
      LF_THROW(broken_receiver_error{});
    }
    m_state->stop.request_stop();
  }

  /**
   * @brief Wait for the associated task to complete and return its result, or rethrow.
   *
   * This may only be called once; the state is consumed and the receiver becomes invalid.
   */
  [[nodiscard]]
  constexpr auto get() && -> T {

    wait();

    // State will be cleaned up on unwind
    std::shared_ptr state = std::exchange(m_state, nullptr);

    LF_ASSUME(state != nullptr);

    if (state->exception) {
      std::rethrow_exception(state->exception);
    }

    if constexpr (!std::is_void_v<T>) {
      return std::move(state->return_value);
    }
  }

 private:
  state_handle<T, Stoppable> m_state;
};

} // namespace lf
