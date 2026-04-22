module;
#include "libfork/__impl/exception.hpp"
export module libfork.batteries:contexts;

import std;

import libfork.core;
import libfork.utils;

import :dummy_stack;

namespace lf {

// =================== Adaptor concepts =================== //

export template <typename A>
concept context_adaptor = worker_context_of<A, handle>;

export template <typename A>
concept posting_context_adaptor = context_adaptor<A> && requires (A &a, handle h) {
  { a.post(h) } -> std::same_as<void>;
};

export template <typename A>
concept stealable_context_adaptor = context_adaptor<A> && requires (A &a) {
  { a.thief() };
};

// =================== Contexts =================== //

export template <worker_stack Stack, context_adaptor Adaptor>
class derived_poly_context : public poly_context<Stack> {
 public:
  using context_type = poly_context<Stack>;

  [[nodiscard]]
  constexpr auto get_underlying() noexcept -> Adaptor & {
    return m_container;
  }

  [[nodiscard]]
  static constexpr auto adopt_steal(handle h) noexcept -> steal_handle<context_type> {
    return {key(), get(key(), h)};
  }

  constexpr void push(steal_handle<context_type> frame) final { m_container.push(frame); }

  constexpr auto pop() noexcept -> steal_handle<context_type> final {
    return {key(), get(key(), m_container.pop())};
  }

  constexpr void post(sched_handle<context_type> h) final {
    if constexpr (posting_context_adaptor<Adaptor>) {
      m_container.post(h);
    } else {
      poly_context<Stack>::post(h);
    }
  }

 private:
  Adaptor m_container;
};

export template <worker_stack Stack, context_adaptor Adaptor>
class mono_context : public base_context<Stack> {
 public:
  using context_type = mono_context;

  [[nodiscard]]
  constexpr auto get_underlying() noexcept -> Adaptor & {
    return m_container;
  }

  [[nodiscard]]
  static constexpr auto adopt_steal(handle h) noexcept -> steal_handle<context_type> {
    return {key(), get(key(), h)};
  }

  constexpr void
  push(steal_handle<context_type> frame) noexcept(noexcept(m_container.push(std::declval<handle>()))) {
    m_container.push(frame);
  }

  constexpr auto pop() noexcept -> steal_handle<context_type> {
    return {key(), get(key(), m_container.pop())};
  }

  constexpr void
  post(sched_handle<context_type> h) noexcept(noexcept(m_container.post(std::declval<handle>())))
    requires posting_context_adaptor<Adaptor>
  {
    m_container.post(h);
  }

 private:
  Adaptor m_container;
};

// TODO: replace dummy_context with unit-context
// TODO: replace dummy_allocator with fixed-allocator

export struct dummy_context {
  void post(lf::sched_handle<dummy_context>);
  void push(lf::steal_handle<dummy_context>);
  auto pop() noexcept -> lf::steal_handle<dummy_context>;
  auto stack() noexcept -> dummy_allocator &;
};

} // namespace lf
