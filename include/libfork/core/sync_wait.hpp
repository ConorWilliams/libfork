#ifndef AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A
#define AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A

#include <semaphore>
#include <type_traits>
#include <utility>

#include <libfork/core/eventually.hpp>
#include <libfork/core/invocable.hpp>

#include <libfork/core/ext/handles.hpp>
#include <libfork/core/ext/list.hpp>
#include <libfork/core/ext/tls.hpp>

#include <libfork/core/impl/combinate.hpp>

namespace lf {

/**
 * @brief A concept that schedulers must satisfy.
 *
 * This requires only a single method, `schedule`.
 */
template <typename Sch>
concept scheduler = requires (Sch &&sch, intruded_list<submit_handle> handle) {
  std::forward<Sch>(sch).schedule(handle); //
};

template <scheduler Sch, async_function_object F, class... Args>
  requires rootable<F, Args...>
auto sync_wait(Sch &&sch, F fun, Args &&...args) -> async_result_t<F, Args...> {

  using namespace lf::impl;
  using R = async_result_t<F, Args...>;
  constexpr bool is_void = std::is_void_v<R>;

  eventually<std::conditional_t<is_void, empty, R>> result;

  bool worker = tls::has_fibre;

  std::optional<fibre> prev = std::nullopt;

  if (!worker) {
    LF_LOG("Sync wait from non-worker thread");
    tls::thread_fibre.construct();
    tls::has_fibre = true;
  } else {
    LF_LOG("Sync wait from worker thread");
    prev.emplace();
    swap(*prev, *tls::thread_fibre);
  }

  quasi_awaitable await = [&]() noexcept(!std::is_trivially_destructible_v<R>) {
    if constexpr (is_void) {
      return combinate<tag::root>(discard_t{}, std::move(fun))(std::forward<Args>(args)...);
    } else {
      return combinate<tag::root>(&result, std::move(fun))(std::forward<Args>(args)...);
    }
  }();

  ignore_t{} = tls::thread_fibre->release();
  // commit_release(tls::thread_fibre->pre_release());

  if (!worker) {
    tls::thread_fibre.destroy();
    tls::has_fibre = false;
  } else {
    swap(*prev, *tls::thread_fibre);
  }

  std::binary_semaphore sem{0};

  await.promise->set_semaphore(&sem);

  auto *handle = std::bit_cast<submit_handle>(static_cast<frame *>(await.promise));

  typename intrusive_list<submit_handle>::node node{handle};

  [&]() noexcept(!std::is_trivially_destructible_v<R>) {
    std::forward<Sch>(sch).schedule(&node);
    sem.acquire();
  }();

  if constexpr (!is_void) {
    return *std::move(result);
  }
}

} // namespace lf

#endif /* AE259086_6D4B_433D_8EEB_A1E8DC6A5F7A */
