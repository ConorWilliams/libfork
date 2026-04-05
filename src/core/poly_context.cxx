module;
#include "libfork/__impl/exception.hpp"
export module libfork.core:poly_context;

import std;

import :concepts;

namespace lf {

template <worker_stack Stack>
class base_context {
 public:
  auto stack() noexcept -> Stack & { return m_stack; }

 protected:
  constexpr base_context() = default;

  template <typename... Args>
    requires std::constructible_from<Stack, Args...> && (sizeof...(Args) > 0)
  explicit(sizeof...(Args) ==
           1) constexpr base_context(Args &&...args) noexcept(std::is_nothrow_constructible_v<Stack, Args...>)
      : m_stack(std::forward<Args>(args)...) {}

 private:
  Stack m_stack;
};

export struct post_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

/**
 * @brief A worker context polymorphic in push/pop/post.
 *
 * This is the canonical/blesses base class in libfork for polymorphic uses
 * cases. Although possible, libfork does not recommend contexts polymorphic
 * in the `.stack` member
 */
export template <worker_stack Stack>
class poly_context : public base_context<Stack> {
 public:
  virtual void push(steal_handle<poly_context>) = 0;
  virtual auto pop() noexcept -> steal_handle<poly_context> = 0;

  virtual void post([[maybe_unused]] sched_handle<poly_context> handle) {
    LF_THROW(post_error{"Derived context does not support posting tasks."});
  }

  virtual ~poly_context() noexcept = default;
};

// TODO: constraints on container

// TODO: could we make the container non template-template
// TODO: allocator aware
// TODO: make post aware

export template <                                    //
    worker_stack Stack,                              //
    template <typename, typename> typename Container //
    >
class generic_poly_context : public poly_context<Stack> {

  using sched_h = sched_handle<poly_context<Stack>>;
  using steal_h = steal_handle<poly_context<Stack>>;

  using allocator_type = Stack::allocator_type;
  using allocator_traits = std::allocator_traits<allocator_type>;

  using container_type = Container<steal_h, typename allocator_traits::template rebind_alloc<steal_h>>;

  /*
   *
   * push/pop optionally post -> needs to know steal/sched handle types
   *
   * construct with what?
   *
   */

 public:
  constexpr void push(steal_h frame) final { m_container.push(frame); }

  constexpr auto pop() noexcept -> steal_h final { return m_container.pop(); }

  // constexpr void post(sched_h frame) {}

  // TODO: make allocator aware
  // TODO: make generic over vector/deque

  // explicit constexpr inline_context(allocator_type const &) noexcept;

 private:
  container_type m_container;
};

} // namespace lf
