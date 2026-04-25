export module libfork.schedulers:inline_scheduler;

import std;

import libfork.core;

import libfork.batteries;

namespace lf {

// TODO: think about initialization:
// - do we need default initializable on stack/context?
// - with allocators

// TODO: Can we store the context directly in TLS?

template <typename Derived, typename Base>
concept derived_context_from = worker_context<Base> && std::derived_from<Derived, Base>;

export template <typename Context>
concept derived_worker_context =
    has_context_typedef<Context> && derived_context_from<Context, context_t<Context>>;

export template <derived_worker_context Context>
class inline_scheduler {
 public:
  using context_type = Context::context_type;

  inline_scheduler() = default;

  template <typename... Args>
    requires std::constructible_from<Context, Args...>
  explicit(sizeof...(Args) == 1)
      inline_scheduler(Args &&...args) noexcept(std::is_nothrow_constructible_v<Context, Args...>)
      : m_context(std::forward<Args>(args)...) {}

  void post(lf::sched_handle<context_type> handle) {
    execute(static_cast<context_type &>(m_context), handle);
  }

 private:
  Context m_context;
};

export template <worker_stack Stack, deque_policy Deque>
using mono_inline_scheduler = inline_scheduler<mono_context<Stack, Deque>>;

export template <worker_stack Stack, deque_policy Deque>
using poly_inline_scheduler = inline_scheduler<derived_poly_context<Stack, Deque>>;

} // namespace lf
