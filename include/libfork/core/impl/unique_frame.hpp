#ifndef A7699F23_E799_46AB_B1E0_7EA36053AD41
#define A7699F23_E799_46AB_B1E0_7EA36053AD41

#include <coroutine> // for coroutine_handle
#include <memory>    // for unique_ptr

#include "libfork/core/impl/frame.hpp"   // for frame
#include "libfork/core/impl/utility.hpp" // for non_null
#include "libfork/core/macro.hpp"        // for LF_STATIC_CALL

/**
 * @file unique_frame.hpp
 *
 * @brief A unique pointer that owns a coroutine frame.
 */

namespace lf::impl {

namespace detail {

struct frame_deleter {
  LF_STATIC_CALL void operator()(frame *frame) noexcept { non_null(frame)->self().destroy(); }
};

} // namespace detail

/**
 * @brief A unique pointer (with a custom deleter) that owns a coroutine frame.
 */
using unique_frame = std::unique_ptr<frame, detail::frame_deleter>;

} // namespace lf::impl

#endif /* A7699F23_E799_46AB_B1E0_7EA36053AD41 */
