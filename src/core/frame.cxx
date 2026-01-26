export module libfork.core:frame;

import std;

namespace lf {

struct frame_type {
  frame_type *parent = nullptr;
};

static_assert(std::is_standard_layout_v<frame_type>);

} // namespace lf
