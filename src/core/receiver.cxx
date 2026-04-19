
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
 * @brief Thrown if the root coroutine frame is too large for the embedded buffer.
 */
export struct root_alloc_error final : libfork_exception {
  [[nodiscard]]
  constexpr auto what() const noexcept -> const char * override {
    return "root coroutine frame exceeds receiver_state buffer size";
  }
};

/**
 * @brief Shared state between a scheduled task and its receiver handle.
 *
 * Internal — users interact with it only through the exported `root_state`
 * wrapper (in schedule.cxx) and the returned `receiver` handle.
 *
 * The aligned `buffer` hosts the root task's coroutine frame via placement
 * `operator new`; the state must outlive the frame, which is arranged by the
 * root promise taking a copy of the `shared_ptr<receiver_state>` parameter.
 *
 * Two distinct `empty_*` tags are used for the potentially-empty members so
 * that `[[no_unique_address]]` can collapse both to the same offset.
 */
template <typename T, bool Stoppable = false>
struct receiver_state {

  /// Size of the embedded coroutine-frame buffer (bytes).
  static constexpr std::size_t buffer_size = 1024;

  struct empty_1 {};
  struct empty_2 {};

  alignas(k_new_align) std::byte buffer[buffer_size]{};

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

/**
 * @brief Lightweight move-only handle owning a pre-allocated root task state.
 *
 * `root_state` is a simple wrapper constructed by the caller and passed by
 * value into `schedule`.  Apart from construction and move-assignment it has
 * no public methods — all user-visible interaction with the scheduled task
 * happens through the `receiver` returned from `schedule`.
 *
 * Construction allocates a `receiver_state<T, Stoppable>` which embeds a
 * 1 KiB aligned buffer; the root coroutine frame is placement-constructed
 * into that buffer by `schedule`.
 *
 * Two constructors are provided, mirroring `make_shared` / `allocate_shared`:
 *   - default-construct: uses `std::make_shared`
 *   - allocator-aware: uses `std::allocate_shared` with the given allocator
 */
export template <typename T, bool Stoppable = false>
class root_state {
 public:
  /// Default: allocate via `std::make_shared`.
  root_state() : m_ptr(std::make_shared<receiver_state<T, Stoppable>>()) {}

  /// Allocator-aware: allocate via `std::allocate_shared` with `alloc`.
  template <typename Allocator>
  root_state(std::allocator_arg_t /*tag*/, Allocator const &alloc)
      : m_ptr(std::allocate_shared<receiver_state<T, Stoppable>>(alloc)) {}

  // Move-only.
  root_state(root_state &&) noexcept = default;
  auto operator=(root_state &&) noexcept -> root_state & = default;
  root_state(root_state const &) = delete;
  auto operator=(root_state const &) -> root_state & = delete;

  ~root_state() = default;

 private:
  [[nodiscard]]
  friend auto get(key_t, root_state &self) noexcept -> std::shared_ptr<receiver_state<T, Stoppable>> & {
    return self.m_ptr;
  }

  std::shared_ptr<receiver_state<T, Stoppable>> m_ptr;
};

export template <typename T, bool Stoppable = false>
class receiver {

  using state_type = receiver_state<T, Stoppable>;

 public:
  constexpr receiver(key_t, std::shared_ptr<state_type> state) noexcept : m_state(std::move(state)) {}

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
   * Only available when Stoppable=true.  Thread-safe.
   */
  constexpr auto request_stop() -> void
    requires Stoppable
  {
    if (!valid()) {
      LF_THROW(broken_receiver_error{});
    }
    m_state->stop.request_stop();
  }

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
  std::shared_ptr<state_type> m_state;
};

} // namespace lf
