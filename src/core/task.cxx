export module libfork.core:task;

import std;

import libfork.utils;

import :concepts;

namespace lf {

/**
 * @brief A type returnable from libfork's async functions/coroutines.
 *
 * This requires that `T` is `void` or a `std::movable` type.
 */
export template <typename T>
concept returnable = std::is_void_v<T> || (plain_object<T> && std::movable<T>);

export template <worker_context>
struct env {
  explicit constexpr env(key_t) noexcept {}
};

// Forward-declare promise_type so task can reference it as a pointer.
export template <returnable T, worker_context Context>
struct promise_type;

/**
 * @brief The return type for libfork's async functions/coroutines.
 *
 * This predominantly exists to disambiguate `libfork`s coroutines from other
 * coroutines and specify `T` the async function's return type which is
 * required to be `void` or a `std::movable` type.
 *
 * \rst
 *
 * .. note::
 *
 *    No consumer of this library should ever touch an instance of this type,
 *    it is used for specifying the return type of an `async` function only.
 *
 * \endrst
 */
export template <returnable T, worker_context Context>
class task {
 public:
  using value_type = T;
  using context_type = Context;

  constexpr task(key_t, promise_type<T, Context> *promise) noexcept : m_promise(promise) {}

 private:
  friend constexpr auto get(key_t, task t) noexcept -> promise_type<T, Context> * { return t.m_promise; }

  promise_type<T, Context> *m_promise;
};

} // namespace lf
