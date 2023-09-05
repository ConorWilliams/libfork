#ifndef C6A18A74_A49E_445C_8900_86E86CCC5F2C
#define C6A18A74_A49E_445C_8900_86E86CCC5F2C

/**
 * @file hack.hpp
 *
 * @brief Compiler specific coroutine frame hacking.
 *
 * \rst
 *
 * A coroutine frame looks a little bit like this:
 *
 * .. code::
 *
 *    struct coroutine_frame {
 *        // maybe some padding [Could be a function of the coroutine body]
 *        void (*resume_fn)();
 *        void (*destroy_fn)();
 *        promise_type promise;
 *        // Other needed variables
 *     };
 *
 * For our purposes we need to pass around a pointer to the first base type
 * of the promise object, this base type must:
 *      1. Have a pointer top of the frame's allocated space to allow a thief to know where the top of the async stack is.
 *      2. Be able to return a coroutine_handle<void> to the current frame.
 *
 * If we know there is no padding and that the coroutine frame is allocated at the start of the allocated space then we can
 * elide both
 *
 * \endrst
 *
 */

#include <atomic>
#include <bit>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

#include "libfork/core/coroutine.hpp"

namespace lf {

struct async_stack : detail::immovable<async_stack> {
  alignas(detail::k_new_align) std::array<std::byte, detail::k_mebibyte> buf;
};

static_assert(std::is_standard_layout_v<async_stack>);

namespace detail {

inline thread_local void *sp;

/**
 * @brief A small struct allocated before each coroutine frame.
 */
class control_block : detail::immovable<control_block> {
public:
  void resume() noexcept {
    LF_LOG("Call to resume on stolen task");

    m_steal += 1;
    m_self.resume();
  }

  //   constexpr void set_frame(void *frame) noexcept {

  //   }

private:
  std::atomic_int32_t m_join = k_i32_max; ///< Number of children joined (with offset).
  std::int32_t m_steal = 0;               ///< Number of steals.
  stdx::coroutine_handle<> m_self;        ///< Handle to this coroutine.
  control_block *m_parent;                ///< Parent task (roots don't have one).
  void *m_frame;                          ///< Pointer to the previous frame, if frame is aligned to __STDCPP_DEFAULT_NEW_ALIGNMENT__ then we don't use this.

  /**
   * We could store m_frame and m_self as offsets from the `this` pointer however we need
   * the control block's size to be a multiple of k_new_align  to allow us to place one
   * before the coroutine frame.
   */
};

static_assert(sizeof(control_block) % k_new_align == 0);

}; // namespace detail

} // namespace lf

#endif /* C6A18A74_A49E_445C_8900_86E86CCC5F2C */
