#ifndef E54125F4_034E_45CD_8DF4_7A71275A5308
#define E54125F4_034E_45CD_8DF4_7A71275A5308

#include <type_traits>

#include "libfork/core/result.hpp"
#include "libfork/core/stack.hpp"
#include "libfork/core/task.hpp"

namespace lf {

template <typename Sch>
concept scheduler = requires(Sch &&sch, frame_block *ext) {
  typename context_of<Sch>;
  std::forward<Sch>(sch).submit(ext);
};

template <typename Context, typename R, stateless F>
struct root_head : basic_first_arg<R, tag::root, F> {
  using context_type = Context;
};

template <typename Context, stateless F, typename... Args>
struct sync_wait_impl {
  using dummy_packet = packet<root_head<Context, void, F>, Args...>;
  using dummy_packet_value_type = value_of<std::invoke_result_t<F, dummy_packet, Args...>>;

  using real_packet = packet<root_head<Context, root_result<dummy_packet_value_type>, F>, Args...>;
  using real_packet_value_type = value_of<std::invoke_result_t<F, real_packet, Args...>>;

  static_assert(std::same_as<dummy_packet_value_type, real_packet_value_type>, "Value type changes!");
};

template <scheduler Sch, stateless F, typename... Args>
using result_t = typename sync_wait_impl<context_of<Sch>, F, Args...>::real_packet_value_type;

template <scheduler Sch, stateless F, typename... Args>
using packet_t = typename sync_wait_impl<context_of<Sch>, F, Args...>::real_packet;

/**
 * @brief The entry point for synchronous execution of asynchronous functions.
 */
template <scheduler Sch, stateless F, class... Args>
auto sync_wait(Sch &&sch, [[maybe_unused]] async<F> fun, Args &&...args) noexcept -> result_t<Sch, F, Args...> {

  root_result<result_t<Sch, F, Args...>> root_block;

  packet_t<Sch, F, Args...> packet{{{root_block}}, std::forward<Args>(args)...};

  frame_block *ext = std::move(packet).invoke();

  LF_LOG("Submitting root");

  std::forward<Sch>(sch).submit(ext);

  LF_LOG("Aquire semaphore");

  root_block.semaphore.acquire();

  LF_LOG("Semaphore acquired");

  if constexpr (!std::is_void_v<result_t<Sch, F, Args...>>) {
    return *std::move(root_block);
  }
}

} // namespace lf

#endif /* E54125F4_034E_45CD_8DF4_7A71275A5308 */
