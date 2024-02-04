#ifndef A7699F23_E799_46AB_B1E0_7EA36053AD41
#define A7699F23_E799_46AB_B1E0_7EA36053AD41

#include <memory>

#include "libfork/core/impl/frame.hpp"
#include "libfork/core/macro.hpp"

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

using unique_frame = std::unique_ptr<frame, detail::frame_deleter>;

} // namespace lf::impl

#endif /* A7699F23_E799_46AB_B1E0_7EA36053AD41 */
