module;
export module libfork.core:context;

import std;

import :frame;

namespace lf {

// TODO: private bits / split
export struct work_handle {
  frame_type *frame;
};

static_assert(std::atomic<work_handle>::is_always_lock_free);

template <typename T>
concept context = requires (T &ctx, work_handle h) {
  { ctx.push(h) };
  { ctx.pop() } noexcept -> std::same_as<work_handle>;
};

export struct polymorphic_context {
  virtual void push(work_handle h) = 0;
  virtual auto pop() noexcept -> work_handle = 0;
  virtual ~polymorphic_context() = default;
};

static_assert(context<polymorphic_context>);

export template <typename Context>
constinit inline thread_local Context *thread_context = nullptr;

} // namespace lf
