export module libfork.core:poly_context;

import std;

import :concepts;

namespace lf {

export template <worker_stack Stack>
class basic_stack_context {
 public:
  auto stack() noexcept -> Stack & { return m_stack; }

 protected:
  constexpr basic_stack_context() = default;

  template <typename... Args>
    requires std::constructible_from<Stack, Args...> && (sizeof...(Args) > 0)
  explicit(sizeof...(Args) == 1) constexpr basic_stack_context(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<Stack, Args...>)
      : m_stack(std::forward<Args>(args)...) {}

 private:
  Stack m_stack;
};

/**
 * @brief A worker context polymorphic in push/pop.
 */
export template <worker_stack Stack>
class basic_poly_context : public basic_stack_context<Stack> {
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
 *  stack method/member.
 *  constructors that forward to the stack's constructors.
 */
export template <bool Polymorphic, worker_stack Stack>
using context_base = std::conditional_t<Polymorphic, basic_poly_context<Stack>, basic_stack_context<Stack>>;

// export using poly_env = env<basic_poly_context<geometric_stack>>;

} // namespace lf
