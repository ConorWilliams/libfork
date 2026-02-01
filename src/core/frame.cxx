module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:frame;

import std;

namespace lf {

enum class category : std::uint8_t {
  fork,
  call,
};

struct cancellation;

struct frame_type {

  frame_type *parent = nullptr; // TODO: set as at root
  cancellation *cancel;

  std::uint32_t merges;       // Atomic is 32 bits for speed
  std::uint16_t steals;       // In debug do overflow checking
  category kind;              // Fork/Call/Just/Root
  std::uint8_t exception_bit; // Atomically set

  [[nodiscard]]
  constexpr auto handle() LF_HOF(std::coroutine_handle<frame_type>::from_promise(*this))
};

static_assert(std::is_standard_layout_v<frame_type>);

} // namespace lf
