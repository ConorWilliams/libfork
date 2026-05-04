export module libfork.core:concepts_semigroup;

import std;

import :concepts_invocable;
import :concepts_indirect;

namespace lf {

// === Semigroup

namespace sync {

template <typename R, typename Fn, typename... Args>
concept invocable_to = std::invocable<Fn, Args...> && std::same_as<std::invoke_result_t<Fn, Args...>, R>;

template <typename R, typename Fn, typename T>
concept semigroup_r =                //
    std::constructible_from<R, T> && //
    invocable_to<R, Fn, T, T> &&     //
    invocable_to<R, Fn, T, R> &&     //
    invocable_to<R, Fn, R, T> &&     //
    invocable_to<R, Fn, R, R>;       //

template <typename R, typename Fn, typename I>
concept indirect_semigroup_r =                                              //
    semigroup_r<R, Fn &, indirect_value_t<I>> &&                            //
    semigroup_r<R, Fn &, std::iter_reference_t<I>> &&                       //
    invocable_to<R, Fn &, indirect_value_t<I>, std::iter_reference_t<I>> && //
    invocable_to<R, Fn &, std::iter_reference_t<I>, indirect_value_t<I>>;   //

export template <typename Fn, typename I>
concept indirect_semigroup =                                                  //
    std::indirectly_readable<I> &&                                            //
    std::copy_constructible<Fn> &&                                            //
    std::regular_invocable<Fn &, indirect_value_t<I>, indirect_value_t<I>> && //
    indirect_semigroup_r<                                                     //
        std::invoke_result_t<Fn &, indirect_value_t<I>, indirect_value_t<I>>, //
        Fn &,                                                                 //
        I                                                                     //
        >;                                                                    //

} // namespace sync

namespace async {

template <typename R, typename Fn, typename Context, typename T>
concept semigroup_r =                           //
    std::constructible_from<R, T> &&            //
    std::default_initializable<R> &&            //
    async_invocable_to<Fn, R, Context, T, T> && //
    async_invocable_to<Fn, R, Context, T, R> && //
    async_invocable_to<Fn, R, Context, R, T> && //
    async_invocable_to<Fn, R, Context, R, R>;   //

template <typename R, typename Fn, typename Context, typename I>
concept indirect_semigroup_r =                                                           //
    semigroup_r<R, Fn, Context, indirect_value_t<I>> &&                                  //
    semigroup_r<R, Fn, Context, std::iter_reference_t<I>> &&                             //
    async_invocable_to<Fn, R, Context, indirect_value_t<I>, std::iter_reference_t<I>> && //
    async_invocable_to<Fn, R, Context, std::iter_reference_t<I>, indirect_value_t<I>>;   //

export template <typename Fn, typename Context, typename I>
concept indirect_semigroup =                                                     //
    std::indirectly_readable<I> &&                                               //
    worker_context<Context> &&                                                   //
    std::copy_constructible<Fn> &&                                               //
    async_invocable<Fn &, Context, indirect_value_t<I>, indirect_value_t<I>> &&  //
    indirect_semigroup_r<                                                        //
        async_result_t<Fn &, Context, indirect_value_t<I>, indirect_value_t<I>>, //
        Fn &,                                                                    //
        Context,                                                                 //
        I                                                                        //
        >;                                                                       //

} // namespace async

export template <typename Fn, typename Context, typename I>
concept indirect_semigroup = async::indirect_semigroup<Fn, Context, I> || sync::indirect_semigroup<Fn, I>;

template <typename Fn, typename Context, typename I>
struct indirect_semigroup_result {
  using type = std::invoke_result_t<Fn &, indirect_value_t<I>, indirect_value_t<I>>;
};

template <typename Fn, typename Context, typename I>
  requires async::indirect_semigroup<Fn, Context, I>
struct indirect_semigroup_result<Fn, Context, I> {
  using type = async_result_t<Fn &, Context, indirect_value_t<I>, indirect_value_t<I>>;
};

export template <typename Fn, typename Context, typename I>
  requires indirect_semigroup<Fn, Context, I>
using indirect_semigroup_t = indirect_semigroup_result<Fn, Context, I>::type;

} // namespace lf
