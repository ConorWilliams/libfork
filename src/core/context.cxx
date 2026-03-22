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
  auto allocator() noexcept -> Alloc & { return m_allocator; }

  virtual void post(await_handle<polymorphic_context>) = 0;
  virtual void push(frame_handle<polymorphic_context>) = 0;
  virtual auto pop() noexcept -> frame_handle<polymorphic_context> = 0;

  virtual ~polymorphic_context() = default;

 protected:
  constexpr polymorphic_context() = default;

  template <typename... Args>
    requires std::constructible_from<Alloc, Args...> && (sizeof...(Args) > 0)
  explicit(sizeof...(Args) == 1) constexpr polymorphic_context(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<Alloc, Args...>)
      : m_allocator(std::forward<Args>(args)...) {}

 private:
  Alloc m_allocator;
};

export template <worker_context Context>
constinit inline thread_local Context *thread_context = nullptr;

} // namespace lf
