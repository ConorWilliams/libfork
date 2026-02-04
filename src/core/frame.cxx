module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:frame;

import std;

namespace lf {

// TODO: remove this and other exports
export enum class category : std::uint8_t {
  call,
  root,
  fork,
};

struct cancellation;

struct frame_type {

  frame_type *parent = nullptr; // TODO: set as at root
  cancellation *cancel = nullptr;

  std::uint32_t merges = 0;       // Atomic is 32 bits for speed
  std::uint16_t steals = 0;       // In debug do overflow checking
  category kind = category::call; // Fork/Call/Just/Root
  std::uint8_t exception_bit = 0; // Atomically set

  [[nodiscard]]
  constexpr auto handle() LF_HOF(std::coroutine_handle<frame_type>::from_promise(*this))
};

static_assert(std::is_standard_layout_v<frame_type>);

} // namespace lf
