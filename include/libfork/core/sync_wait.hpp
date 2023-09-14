#ifndef E54125F4_034E_45CD_8DF4_7A71275A5308
#define E54125F4_034E_45CD_8DF4_7A71275A5308

#include "libfork/core/result.hpp"
#include "libfork/core/stack.hpp"
#include "libfork/core/task.hpp"

namespace lf {

template <thread_context Context, typename Head, class... Args>
auto sync_wait_impl(Context &scheduler, Head head, Args &&...args) -> typename packet<Head, Args...>::value_type {

  // using packet_t = packet<Head, Args...>;

  using value_type = typename packet<Head, Args...>::value_type;

  using wrapped_value_type = std::conditional_t<std::is_reference_v<value_type>,
                                                std::reference_wrapper<std::remove_reference_t<value_type>>, value_type>;

  struct wrap : Head {
    using return_address_t = root_block_t<wrapped_value_type>;
  };

  using packet_type = packet<wrap, Args...>;

  static_assert(std::same_as<value_type, typename packet_type::value_type>,
                "An async function's value_type must be return_address_t independent!");

  typename wrap::return_address_t root_block;

  auto handle = packet_type{root_block, {std::move(head)}, {std::forward<Args>(args)...}}.invoke_bind(nullptr);

  LF_TRY { std::forward<Schedule>(scheduler).schedule(stdx::coroutine_handle<>{handle}); }
  LF_CATCH_ALL {
    // We cannot know whether the coroutine has been resumed or not once we pass to schedule(...).
    // Hence, we do not know whether or not to .destroy() if schedule(...) threw.
    // Hence we mark noexcept to trigger termination.
    detail::noexcept_invoke([] { LF_RETHROW; });
  }

  // Block until the coroutine has finished.
  root_block.semaphore.acquire();

  root_block.exception.rethrow_if_unhandled();

  if constexpr (!std::is_void_v<value_type>) {
    LF_ASSERT(root_block.result.has_value());
    return std::move(*root_block.result);
  }
}

template <typename Context, typename R, stateless F>
struct root_head : basic_first_arg<R, tag::root, F> {
  using context_type = Context;
};

template <typename Context, stateless F, typename... Args>
struct sync_wait_impl_2 {
  using dummy_packet = packet<root_head<Context, void, F>, Args...>;
  using dummy_packet_value_type = value_of<std::invoke_result_t<F, dummy_packet, Args...>>;

  using real_packet = packet<root_head<Context, root_result<dummy_packet_value_type>, F>, Args...>;
  using real_packet_value_type = value_of<std::invoke_result_t<F, real_packet, Args...>>;

  static_assert(std::same_as<dummy_packet_value_type, real_packet_value_type>, "Value type changes!");
};

template <typename Context, stateless F, typename... Args>
using result_t = typename sync_wait_impl_2<Context, F, Args...>::real_packet_value_type;

template <typename Context, stateless F, typename... Args>
using packet_t = typename sync_wait_impl_2<Context, F, Args...>::real_packet;

/**
 * @brief The entry point for synchronous execution of asynchronous functions.
 */
template <thread_context Context, stateless F, class... Args>
auto sync_wait(Context &context, [[maybe_unused]] async<F> fun, Args &&...args) noexcept -> result_t<Context, F, Args...> {

  root_result<result_t<Context, F, Args...>> root_block;

  packet_t<Context, F, Args...> packet{{{root_block}}, std::forward<Args>(args)...};

  frame_block *ext = std::move(packet).invoke();

  context.submit(ext);

  root_block.semaphore.acquire();

  if constexpr (!std::is_void_v<result_t<Context, F, Args...>>) {
    return *std::move(root_block);
  }
}

} // namespace lf

#endif /* E54125F4_034E_45CD_8DF4_7A71275A5308 */
