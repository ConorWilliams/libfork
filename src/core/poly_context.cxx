export module libfork.core:poly_context;

import std;

import :concepts;

namespace lf {

export template <stack_allocator Alloc>
class basic_stack_context {
 public:
  auto allocator() noexcept -> Alloc & { return m_allocator; }

 protected:
  constexpr basic_stack_context() = default;

  template <typename... Args>
    requires std::constructible_from<Alloc, Args...> && (sizeof...(Args) > 0)
  explicit(sizeof...(Args) == 1) constexpr basic_stack_context(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<Alloc, Args...>)
      : m_allocator(std::forward<Args>(args)...) {}

 private:
  Alloc m_allocator;
};

/**
 * @brief A worker context polymorphic in push/pop.
 */
export template <stack_allocator Alloc>
class basic_poly_context : public basic_stack_context<Alloc> {
 public:
  virtual void post(sched_handle<basic_poly_context>) = 0;
  virtual void push(steal_handle<basic_poly_context>) = 0;
  virtual auto pop() noexcept -> steal_handle<basic_poly_context> = 0;

  virtual ~basic_poly_context() noexcept = default;
};

/**
 * @brief A potentially polymorphic worker context base class.
 *
 * Provides:
 *  virtual push/pop/post/deleter if Polymorphic is true, otherwise provides no virtual functions.
 *  allocator method/member.
 *  constructors that forward to the allocator's constructors.
 */
export template <bool Polymorphic, stack_allocator Alloc>
using context_base = std::conditional_t<Polymorphic, basic_poly_context<Alloc>, basic_stack_context<Alloc>>;

// export using poly_env = env<basic_poly_context<geometric_stack>>;

} // namespace lf
