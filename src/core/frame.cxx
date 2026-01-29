module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:frame;

import std;

namespace lf {

struct frame_type {

  frame_type *parent = nullptr;

  [[nodiscard]]
  constexpr auto handle() LF_HOF(std::coroutine_handle<frame_type>::from_promise(*this))
};

static_assert(std::is_standard_layout_v<frame_type>);

} // namespace lf
