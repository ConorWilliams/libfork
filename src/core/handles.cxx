export module libfork.core:handles;

import std;

import libfork.utils;

import :frame;

namespace lf {

// =================== Untyped handle =================== //
//
// `handle` is the untyped storage form — trivially-copyable pointer-sized, suitable for atomic storage.
// Only libfork internals can construct one or extract the frame pointer, via the `key_t` passkey.

export class handle {
 public:
  constexpr handle() = default;
  constexpr handle(key_t, frame_base *ptr) noexcept : m_ptr{ptr} {}
  constexpr auto operator==(handle const &) const noexcept -> bool = default;
  constexpr explicit operator bool() const noexcept { return m_ptr != nullptr; }

 private:
  [[nodiscard]]
  constexpr friend auto get(key_t, handle h) noexcept -> frame_base * {
    return h.m_ptr;
  }

  frame_base *m_ptr = nullptr;
};

// =================== Tagged handles =================== //
//
// Tagged wrappers carrying the context type at API boundaries. They slice to `handle`
// for untyped storage; re-tagging from `handle` is gated on `key_t`.

// TODO: when a steal handle resumes a task and increments the number of forks
// it should check and potentially crash if the number of forks exceeds the
// maximum determined by uint16_max

export template <typename T>
struct steal_handle : handle {
  using handle::handle;
};

export template <typename T>
struct sched_handle : handle {
  using handle::handle;
};

} // namespace lf
