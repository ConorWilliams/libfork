
export module libfork.core:concepts_awaitable;

import :handles;
import :concepts_context;

namespace lf {

template <typename T, typename Context>
concept awaitable = worker_context<Context> && requires (T &&val, Context &ctx, sched_handle<Context> h) {
  { static_cast<T &&>(val).await_ready() } -> std::convertible_to<bool>;
  { static_cast<T &&>(val).await_suspend(h, ctx) } -> std::same_as<void>;
  { static_cast<T &&>(val).await_resume() };
};

} // namespace lf
