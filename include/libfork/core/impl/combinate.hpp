#ifndef AD9A2908_3043_4CEC_9A2A_A57DE168DF19
#define AD9A2908_3043_4CEC_9A2A_A57DE168DF19

#include <type_traits>
#include <utility>

#include "libfork/core/first_arg.hpp"
#include "libfork/core/invocable.hpp"

#include "libfork/core/impl/awaitables.hpp"

namespace lf::impl {

// ---------------------------- //

template <typename R, quasi_pointer I, tag Tag>
struct promise_type;

// ---------------------------- //

template <tag Tag, quasi_pointer I, async_function_object F>
struct [[nodiscard("A bound function SHOULD be immediately invoked!")]] y_combinate {

  [[no_unique_address]] I ret; ///< The return address.
  [[no_unique_address]] F fun; ///< The asynchronous function.

  /**
   * @brief Invoke the coroutine, set's the return pointer.
   */
  template <typename... Args>
    requires async_invocable<I, Tag, F, Args...>
  auto operator()(auto &&...args) -> quasi_awaitable {

    task task = std::invoke(                                             //
        std::move(fun),                                                  //
        first_arg_t<I, Tag, std::remove_cvref_t<F>>(std::as_const(fun)), //
        std::forward<Args>(args)...                                      //
    );                                                                   //

    using value_type = unsafe_result_t<I, Tag, F, Args...>;

    auto *prom = static_cast<promise_type<value_type, I, Tag> *>(task.promise);

    prom->set_return(std::move(ret));

    return {prom};
  }
};

// // ---------------------------- //

template <tag Tag, quasi_pointer I, async_function_object F>
auto combinate(I ret, F fun) -> y_combinate<Tag, I, F> {
  return {std::move(ret), std::move(fun)};
}

/**
 * @brief Prevent each layer wrapping the function in another `first_arg_t`.
 */
template <tag Tag, tag OtherTag, quasi_pointer I, quasi_pointer OtherI, async_function_object F>
auto combinate(I ret, first_arg_t<OtherI, OtherTag, F> arg) -> y_combinate<Tag, I, F> {
  return {std::move(ret), unwrap(std::move(arg))};
}

} // namespace lf::impl

#endif /* AD9A2908_3043_4CEC_9A2A_A57DE168DF19 */