#ifndef E816E4D9_BBF7_41E6_923F_332F6E8AF548
#define E816E4D9_BBF7_41E6_923F_332F6E8AF548

#include <algorithm>
#include <iterator>
#include <ranges>

#include "libfork/core.hpp"

namespace lf {

namespace detail {

inline constexpr async_fn for_each =
    []<std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
       std::indirectly_unary_invocable<std::projected<I, Proj>> Fun>(
        auto for_each, I head, S tail, Fun f, Proj proj = {}, std::iter_difference_t<I> chunk = 1)
        LF_STATIC_CALL->lf::task<void> {
  //
  auto len = head - tail;

  if (len <= chunk) {
    std::for_each(head, tail, f, proj);
    co_return;
  }

  auto mid = head + len / 2;

  co_await lf::fork(for_each)(head, mid, f, proj, chunk);
  co_await lf::call(for_each)(mid, tail, f, proj, chunk);

  co_await lf::join;
};

// inline constexpr auto wrap = lf::async([]<class F, class... Args>(auto self, F &&f, Args &&...args) ->
// lf::task<std::invoke_result_t<F, Args...>> {
//   co_return std::invoke(std::forward<decltype(f)>(f), std::forward<decltype(args)>(args)...);
// });

// }

// inline constexpr auto for_each = lf::async([](auto for_each, auto beg, auto end, auto const &fun,
// std::ptrdiff_t const &chunk_size) -> lf::task<void> {
//   if
// });

} // namespace detail

} // namespace lf

#endif /* E816E4D9_BBF7_41E6_923F_332F6E8AF548 */
