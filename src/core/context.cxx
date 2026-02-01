module;
export module libfork.core:context;

import std;

import :frame;

namespace lf {

export struct work_handle {
  frame_type *frame;
};

template <typename Context>
constinit inline thread_local Context *thread_context = nullptr;

} // namespace lf
