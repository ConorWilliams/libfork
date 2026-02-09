module;
export module libfork.core:context;

import std;

import :concepts;

namespace lf {

export template <stack_allocator Alloc>
class polymorphic_context {
 public:
  auto alloc() noexcept -> Alloc & { return m_allocator; }
  virtual void push(frame_handle<polymorphic_context>) = 0;
  virtual auto pop() noexcept -> frame_handle<polymorphic_context> = 0;
  virtual ~polymorphic_context() = default;

 private:
  Alloc m_allocator;
};

// static_assert(context<polymorphic_context>);

export template <worker_context Context>
constinit inline thread_local Context *thread_context = nullptr;

} // namespace lf
