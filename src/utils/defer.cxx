module;
#include "libfork/__impl/compiler.hpp"
export module libfork.utils:defer;

import std;

namespace lf {

/**
 * @brief Basic implementation of a Golang-like defer.
 *
 * \rst
 *
 * Use like:
 *
 * .. code::
 *
 *    auto * ptr = c_api_init();
 *
 *    defer _ = [&ptr] noexcept {
 *      c_api_clean_up(ptr);
 *    };
 *
 *    // Code that may throw
 *
 * \endrst
 */
export template <class Fn>
  requires std::is_nothrow_invocable_v<Fn> && std::is_object_v<Fn>
class [[nodiscard("Defer will execute unless bound to a name!")]] defer {
 public:
  defer(defer const &) = delete;
  defer(defer &&) = delete;
  auto operator=(defer const &) -> defer & = delete;
  auto operator=(defer &&) -> defer & = delete;

  /**
   * @brief Construct a new Defer object.
   *
   * @param fn Nullary invocable forwarded into object and invoked by destructor.
   */
  constexpr defer(Fn &&fn) noexcept(std::is_nothrow_constructible_v<Fn, Fn &&>)
      : m_fn(std::forward<Fn>(fn)) {}

  /**
   * @brief Calls the invocable.
   */
  LF_FORCE_INLINE constexpr ~defer() noexcept { std::invoke(std::move(m_fn)); }

 private:
  [[no_unique_address]]
  Fn m_fn;
};

} // namespace lf
