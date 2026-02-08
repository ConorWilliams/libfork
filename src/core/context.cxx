module;
export module libfork.core:context;

import std;

import :concepts;

namespace lf {

export template <stack_allocator Alloc>
class polymorphic_context {
 public:
  auto stack() -> Alloc & { return m_allocator; }
  virtual void push(frame_handle<polymorphic_context> h) = 0;
  virtual auto pop() noexcept -> frame_handle<polymorphic_context> = 0;
  virtual ~polymorphic_context() = default;

 private:
  Alloc m_allocator;
};

// static_assert(context<polymorphic_context>);

} // namespace lf
