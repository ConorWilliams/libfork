export module libfork.core:poly_context;

import std;

import :concepts;

namespace lf {

/**
 * @brief A worker context polymorphic in push/pop.
 */
export template <stack_allocator Alloc>
class basic_poly_context {
 public:
  using checkpoint_type = decltype(std::declval<Alloc &>().checkpoint());

  auto allocator() noexcept -> Alloc & { return m_allocator; }

  virtual void post(await_handle<checkpoint_type>) = 0;
  virtual void push(frame_handle<checkpoint_type>) = 0;
  virtual auto pop() noexcept -> frame_handle<checkpoint_type> = 0;

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
