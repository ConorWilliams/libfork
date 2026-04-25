module;
#include "libfork/__impl/exception.hpp"
export module libfork.batteries:contexts;

import std;

import libfork.core;
import libfork.utils;

import :dummy_stack;

namespace lf {

// =================== Context Policies =================== //

/**
 * @brief The simplest context policy is just a LIFO stack of type-erased handles.
 *
 * Context policies (unlike full contexts) are not aware of the full context
 * type hence, operate on untyped handles. This is inherently unsafe. To
 * prevent UB a policy must not give-out the handles it receives. All
 * operations must be managed through either `derived_poly_context` or
 * `mono_context`.
 */
export template <typename T>
concept deque_policy = lifo_stack<T, unsafe_steal_handle>;

// TODO: consider the methods/concepts needed for a auto/scheduling worker
// context that has a `post` method.

/**
 * @brief An extension of `deque_policy` that supports FIFO stealing of handles.
 */
export template <typename T>
concept stealable_deque_policy = deque_policy<T> && requires (T &policy) {
  { policy.steal() } -> std::same_as<unsafe_steal_handle>;
};

// =================== Contexts =================== //

/**
 * @brief A polymorphic worker context composed of a `worker_stack` and a `deque_policy`.
 */
export template <worker_stack Stack, deque_policy Deque>
class derived_poly_context : public poly_context<Stack> {
 public:
  using context_type = poly_context<Stack>;

  constexpr derived_poly_context() = default;

  template <typename... StackArgs, typename... DequeArgs>
    requires std::constructible_from<Stack, StackArgs...> && std::constructible_from<Deque, DequeArgs...>
  constexpr derived_poly_context(
      std::piecewise_construct_t,
      std::tuple<StackArgs...> stack_args,
      std::tuple<DequeArgs...> deque_args) noexcept(std::is_nothrow_constructible_v<Stack, StackArgs...> &&
                                                    std::is_nothrow_constructible_v<Deque, DequeArgs...>)
      : derived_poly_context(std::move(stack_args),
                             std::move(deque_args),
                             std::index_sequence_for<StackArgs...>{},
                             std::index_sequence_for<DequeArgs...>{}) {}

  constexpr void push(steal_handle<context_type> handle) final { m_container.push(handle); }

  constexpr auto pop() noexcept -> steal_handle<context_type> final {
    return {key(), get(key(), m_container.pop())};
  }

  [[nodiscard]]
  constexpr auto steal() noexcept(noexcept(m_container.steal())) -> steal_handle<context_type>
    requires stealable_deque_policy<Deque>
  {
    return {key(), get(key(), m_container.steal())};
  }

 private:
  template <typename... StackArgs, typename... DequeArgs, std::size_t... Is, std::size_t... Js>
  constexpr derived_poly_context(
      std::tuple<StackArgs...> stack_args,
      std::tuple<DequeArgs...> deque_args,
      std::index_sequence<Is...>,
      std::index_sequence<Js...>) noexcept(std::is_nothrow_constructible_v<Stack, StackArgs...> &&
                                           std::is_nothrow_constructible_v<Deque, DequeArgs...>)
      : poly_context<Stack>(std::get<Is>(std::move(stack_args))...),
        m_container(std::get<Js>(std::move(deque_args))...) {}

  Deque m_container;
};

export template <worker_stack Stack, deque_policy Deque>
class mono_context : public base_context<Stack> {
 public:
  using context_type = mono_context;

  constexpr mono_context() = default;

  template <typename... StackArgs, typename... DequeArgs>
    requires std::constructible_from<Stack, StackArgs...> && std::constructible_from<Deque, DequeArgs...>
  constexpr mono_context(
      std::piecewise_construct_t,
      std::tuple<StackArgs...> stack_args,
      std::tuple<DequeArgs...> deque_args) noexcept(std::is_nothrow_constructible_v<Stack, StackArgs...> &&
                                                    std::is_nothrow_constructible_v<Deque, DequeArgs...>)
      : mono_context(std::move(stack_args),
                     std::move(deque_args),
                     std::index_sequence_for<StackArgs...>{},
                     std::index_sequence_for<DequeArgs...>{}) {}

  constexpr void push(steal_handle<context_type> handle) noexcept(noexcept(m_container.push(handle))) {
    m_container.push(handle);
  }

  constexpr auto pop() noexcept -> steal_handle<context_type> {
    return {key(), get(key(), m_container.pop())};
  }

  [[nodiscard]]
  constexpr auto steal() noexcept(noexcept(m_container.steal())) -> steal_handle<context_type>
    requires stealable_deque_policy<Deque>
  {
    return {key(), get(key(), m_container.steal())};
  }

 private:
  template <typename... StackArgs, typename... DequeArgs, std::size_t... Is, std::size_t... Js>
  constexpr mono_context(
      std::tuple<StackArgs...> stack_args,
      std::tuple<DequeArgs...> deque_args,
      std::index_sequence<Is...>,
      std::index_sequence<Js...>) noexcept(std::is_nothrow_constructible_v<Stack, StackArgs...> &&
                                           std::is_nothrow_constructible_v<Deque, DequeArgs...>)
      : base_context<Stack>(std::get<Is>(std::move(stack_args))...),
        m_container(std::get<Js>(std::move(deque_args))...) {}

  Deque m_container;
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
