module;
export module libfork.context:inline_context;

import std;

import libfork.core;

namespace lf {

export template <stack_allocator Stack>
class inline_context {
  using allocator_type = Stack::allocator_type;
  using allocator_traits = std::allocator_traits<allocator_type>;
  using allocator_handle = allocator_traits::template rebind_alloc<frame_handle<inline_context>>;

 public:
  constexpr void post(await_handle<inline_context>) {}

  constexpr void push(frame_handle<inline_context>) {}

  constexpr auto pop() noexcept -> frame_handle<inline_context>;

  constexpr auto allocator() noexcept -> inline_context &;

  explicit constexpr inline_context(allocator_type const &) noexcept;

 private:
  Stack m_allocator;
  std::vector<frame_handle<inline_context>, allocator_handle> m_stack;
};

} // namespace lf
