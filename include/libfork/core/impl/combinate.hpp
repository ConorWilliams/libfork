#ifndef AD9A2908_3043_4CEC_9A2A_A57DE168DF19
#define AD9A2908_3043_4CEC_9A2A_A57DE168DF19

#include <type_traits>
#include <utility>

#include "libfork/core/first_arg.hpp"
#include "libfork/core/invocable.hpp"

#include "libfork/core/impl/awaitables.hpp"

namespace lf::impl {

// ---------------------------- //

template <returnable R, return_address_for<R> I, tag Tag>
struct promise;

// -------------------------------------------------------- //

/**
 * @brief Awaitable in the context of an `lf::task` coroutine.
 */
template <returnable R, return_address_for<R> I, tag Tag>
struct [[nodiscard("A quasi_awaitable MUST be immediately co_awaited!")]] quasi_awaitable {
  promise<R, I, Tag> *promise; ///< The parent/semaphore needs to be set!
};

// ---------------------------- //

template <quasi_pointer I, tag Tag, async_function_object F>
struct [[nodiscard("A bound function SHOULD be immediately invoked!")]] y_combinate {

  [[no_unique_address]] I ret; ///< The return address.
  [[no_unique_address]] F fun; ///< The asynchronous function.

  /**
   * @brief Invoke the coroutine, set's the return pointer.
   */
  template <typename... Args>
    requires async_invocable<I, Tag, F, Args...>
  auto operator()(Args &&...args) && -> quasi_awaitable<async_result_t<F, Args...>, I, Tag> {

    task task = std::move(fun)(                                  //                                   
        first_arg_t<I, Tag, F, Args &&...>(std::as_const(fun)),  // 
        std::forward<Args>(args)...                              //
    );

    using R = async_result_t<F, Args...>;
    using P = promise<R, I, Tag>;

    auto *prom = static_cast<P *>(task.promise);

    if constexpr (!std::is_void_v<R>) {
      prom->set_return(std::move(ret));
    }

    return {prom};
  }
};

// // ---------------------------- //

template <tag Tag, quasi_pointer I, async_function_object F>
auto combinate(I ret, F fun) -> y_combinate<I, Tag, F> {
  return {std::move(ret), std::move(fun)};
}

/**
 * @brief Prevent each layer wrapping the function in another `first_arg_t`.
 */
template <tag Tag,
          tag OtherTag,
          quasi_pointer I,
          quasi_pointer OtherI,
          async_function_object F,
          typename... Args>
auto combinate(I ret, first_arg_t<OtherI, OtherTag, F, Args...> arg) -> y_combinate<I, Tag, F> {
  return {std::move(ret), unwrap(std::move(arg))};
}

} // namespace lf::impl

#endif /* AD9A2908_3043_4CEC_9A2A_A57DE168DF19 */
