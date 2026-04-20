module;
#include "libfork/__impl/exception.hpp"
export module libfork.batteries:contexts;

import std;

import libfork.core;

import :dummy_stack;

namespace lf {

// TODO: constraints on container

// TODO: could we make the container non template-template
// TODO: allocator aware
// TODO: make post aware

export template <                        //
    worker_stack Stack,                  //
    template <typename> typename Adaptor //
    >
class derived_poly_context : public poly_context<Stack> {
 public:
  using context_type = poly_context<Stack>;

  constexpr derived_poly_context() = default;

  template <typename StackTuple, typename AdaptorTuple>
  constexpr derived_poly_context(std::piecewise_construct_t,
                                 StackTuple &&stack_args,
                                 AdaptorTuple &&adaptor_args)
      : poly_context<Stack>(std::make_from_tuple<Stack>(std::forward<StackTuple>(stack_args))),
        m_container(
            std::make_from_tuple<Adaptor<context_type>>(std::forward<AdaptorTuple>(adaptor_args))) {}

  [[nodiscard]]
  constexpr auto get_underlying() noexcept -> Adaptor<context_type> & {
    return m_container;
  }

  constexpr void push(steal_handle<context_type> frame) final { m_container.push(frame); }

  constexpr auto pop() noexcept -> steal_handle<context_type> final { return m_container.pop(); }

  constexpr void post(sched_handle<context_type> handle) final {

    constexpr bool has_post = requires {
      { m_container.post(handle) } -> std::same_as<void>;
    };

    if constexpr (has_post) {
      m_container.post(handle);
    } else {
      poly_context<Stack>::post(handle);
    }
  }

 private:
  Adaptor<context_type> m_container;
};

// TODO: allow customization of post (via Container?)
// TODO: allocator aware

export template <                        //
    worker_stack Stack,                  //
    template <typename> typename Adaptor //
    >
class mono_context : public base_context<Stack> {
 public:
  using context_type = mono_context;

  constexpr mono_context() = default;

  template <typename StackTuple, typename AdaptorTuple>
  constexpr mono_context(std::piecewise_construct_t,
                         StackTuple &&stack_args,
                         AdaptorTuple &&adaptor_args)
      : base_context<Stack>(std::make_from_tuple<Stack>(std::forward<StackTuple>(stack_args))),
        m_container(
            std::make_from_tuple<Adaptor<context_type>>(std::forward<AdaptorTuple>(adaptor_args))) {}

  [[nodiscard]]
  constexpr auto get_underlying() noexcept -> Adaptor<context_type> & {
    return m_container;
  }

  constexpr void push(steal_handle<context_type> frame) noexcept(noexcept(m_container.push(frame))) {
    m_container.push(frame);
  }

  constexpr auto pop() noexcept -> steal_handle<context_type> { return m_container.pop(); }

  constexpr void post(sched_handle<context_type> handle) noexcept(noexcept(m_container.post(handle)))
    requires requires (Adaptor<context_type> context) {
      { context.post(handle) } -> std::same_as<void>;
    }
  {
    m_container.post(handle);
  }

 private:
  Adaptor<context_type> m_container;
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
