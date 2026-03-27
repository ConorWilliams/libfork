export module libfork.core:poly_context;

import std;

import :concepts;

namespace lf {

// TODO: is it possible to simultaniosly be poly context and concrete context?

/**
 * @brief A worker context polymorphic in push/pop.
 */
export template <stack_allocator Alloc>
class basic_poly_context {
 public:
  auto allocator() noexcept -> Alloc & { return m_allocator; }

  virtual void post(await_handle<basic_poly_context>) = 0;
  virtual void push(frame_handle<basic_poly_context>) = 0;
  virtual auto pop() noexcept -> frame_handle<basic_poly_context> = 0;

  virtual ~basic_poly_context() = default;

 protected:
  constexpr basic_poly_context() = default;

  template <typename... Args>
    requires std::constructible_from<Alloc, Args...> && (sizeof...(Args) > 0)
  explicit(sizeof...(Args) == 1) constexpr basic_poly_context(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<Alloc, Args...>)
      : m_allocator(std::forward<Args>(args)...) {}

 private:
  Alloc m_allocator;
};

// export using poly_env = env<basic_poly_context<geometric_stack>>;

} // namespace lf
