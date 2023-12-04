#ifndef B4EE570B_F5CF_42CB_9AF3_7376F45FDACC
#define B4EE570B_F5CF_42CB_9AF3_7376F45FDACC

#include <concepts>
#include <functional>
#include <libfork/core/macro.hpp>
#include <type_traits>
#include <utility>

#include "libfork/core/impl/utility.hpp"

namespace lf {

/**
 * @brief Basic implementation of a Golang like defer.
 *
 * \rst
 *
 * Use like:
 *
 * .. code::
 *
 *    auto * ptr = c_api_init();
 *
 *    defer _ = [&ptr] () noexcept {
 *      c_api_clean_up(ptr);
 *    };
 *
 *    // Code that may throw
 *
 * \endrst
 *
 * You can also use the ``LF_DEFER`` macro to create an automatically named defer object.
 *
 */
template <class F>
  requires std::is_nothrow_invocable_v<F>
class [[nodiscard("Defer will execute unless bound to a name!")]] defer : impl::immovable<defer<F>> {
 public:
  /**
   * @brief Construct a new Defer object.
   *
   * @param f Nullary invocable forwarded into object and invoked by destructor.
   */
  constexpr defer(F &&f) noexcept(std::is_nothrow_constructible_v<F, F &&>) : m_f(std::forward<F>(f)) {}

  /**
   * @brief Calls the invocable.
   */
  LF_FORCEINLINE constexpr ~defer() noexcept { std::invoke(std::forward<F>(m_f)); }

 private:
  [[no_unique_address]] F m_f;
};

#define LF_CONCAT_OUTER(a, b) LF_CONCAT_INNER(a, b)
#define LF_CONCAT_INNER(a, b) a##b

/**
 * @brief A macro to create an automatically named defer object.
 */
#define LF_DEFER ::lf::defer LF_CONCAT_OUTER(at_exit_, __LINE__) = [&]() noexcept

} // namespace lf

#endif /* B4EE570B_F5CF_42CB_9AF3_7376F45FDACC */
