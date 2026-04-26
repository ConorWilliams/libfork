export module libfork.core:concepts_scheduler;

import std;

import :handles;

namespace lf {

export template <typename T>
concept has_context_typedef = requires { typename std::remove_cvref_t<T>::context_type; };

export template <has_context_typedef T>
using context_t = typename std::remove_cvref_t<T>::context_type;

/**
 * @brief An object capable of scheduling a libfork task for execution.
 *
 * These are typed to a context, the `post` method must:
 *
 * - Satisfy the strong exception guarantee.
 * - Guarantee eventual execution of the task associated with `handle`.
 */
export template <typename Sch>
concept scheduler =
    has_context_typedef<Sch> && requires (Sch &&scheduler, sched_handle<context_t<Sch>> handle) {
      { static_cast<Sch &&>(scheduler).post(handle) } -> std::same_as<void>;
    };

} // namespace lf
