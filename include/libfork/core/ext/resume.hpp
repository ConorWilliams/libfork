#ifndef DE9399DB_593B_4C5C_A9D7_89B9F2FAB920
#define DE9399DB_593B_4C5C_A9D7_89B9F2FAB920

#include "libfork/core/ext/tls.hpp"

#include "libfork/core/ext/handles.hpp"

namespace lf {

inline namespace ext {

/**
 * @brief Resume a task at a submission point.
 */
inline void resume(submit_handle ptr) noexcept {

  LF_LOG("Call to resume on submitted task");

  auto *frame = std::bit_cast<impl::frame *>(ptr);

  *impl::tls::fibre() = fibre{frame->fibril()};

  frame->self().resume();
}

/**
 * @brief Resume a stolen task.
 */
inline void resume(task_handle ptr) noexcept {

  LF_LOG("Call to resume on stolen task");

  auto *frame = std::bit_cast<impl::frame *>(ptr);

  frame->fetch_add_steal();

  frame->self().resume();
}

} // namespace ext

} // namespace lf

#endif /* DE9399DB_593B_4C5C_A9D7_89B9F2FAB920 */
