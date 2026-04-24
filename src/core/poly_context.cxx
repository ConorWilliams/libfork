module;
#include "libfork/__impl/exception.hpp"
export module libfork.core:poly_context;

import std;

import libfork.utils;

import :concepts_stack;
import :handles;

namespace lf {

export template <worker_stack Stack>
class base_context {
 public:
  auto stack() noexcept -> Stack & { return m_stack; }

 protected:
  constexpr base_context() = default;

  template <typename... Args>
    requires std::constructible_from<Stack, Args...>
  explicit(sizeof...(Args) ==
           1) constexpr base_context(Args &&...args) noexcept(std::is_nothrow_constructible_v<Stack, Args...>)
      : m_stack(std::forward<Args>(args)...) {}

 private:
  Stack m_stack;
};

export struct post_error final : libfork_exception {
  [[nodiscard]]
  constexpr auto what() const noexcept -> const char * override {
    return "derived context does not support posting tasks.";
  }
};

/**
 * @brief A worker context polymorphic in push/pop/post.
 *
 * This is the canonical/blessed base class in libfork for polymorphic uses
 * cases. Although possible, libfork does not recommend contexts polymorphic
 * in the `.stack` member
 */
export template <worker_stack Stack>
class poly_context : public base_context<Stack> {
 public:
  virtual void push(steal_handle<poly_context>) = 0;
  virtual auto pop() noexcept -> steal_handle<poly_context> = 0;

  // TODO: re-introdcue post API if/when we support auto/scheduling contexts.
  // virtual void post([[maybe_unused]] sched_handle<poly_context> handle) { LF_THROW(post_error{}); }

  virtual ~poly_context() noexcept = default;
};

} // namespace lf
