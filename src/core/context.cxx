export module libfork.core:context;

import std;

import :concepts;

namespace lf {

/**
 * @brief A worker context polymorphic in push/pop.
 */
export template <stack_allocator Alloc>
class polymorphic_context {
 public:
  constexpr polymorphic_context() = default;

  explicit constexpr polymorphic_context(Alloc alloc) noexcept(std::is_nothrow_move_constructible_v<Alloc>)
      : m_allocator(std::move(alloc)) {}

  virtual void post(await_handle<polymorphic_context>) = 0;
  virtual void push(frame_handle<polymorphic_context>) = 0;
  virtual auto pop() noexcept -> frame_handle<polymorphic_context> = 0;

  auto alloc() noexcept -> Alloc & { return m_allocator; }

  virtual ~polymorphic_context() = default;

 private:
  Alloc m_allocator;
};

export template <worker_context Context>
constinit inline thread_local Context *thread_context = nullptr;

} // namespace lf
