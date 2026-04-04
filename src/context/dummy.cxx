module;
export module libfork.context:dummy_context;

import std;

import libfork.core;

namespace lf {

// TODO: replace dummy_context with unit-context

// TODO: replace dummy_allocator with fixed-allocator

export struct dummy_context {
  void post(lf::sched_handle<dummy_context>);
  void push(lf::steal_handle<dummy_context>);
  auto pop() noexcept -> lf::steal_handle<dummy_context>;
  auto stack() noexcept -> dummy_allocator &;
};

} // namespace lf
