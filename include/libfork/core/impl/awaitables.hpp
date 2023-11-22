#ifndef CF3E6AC4_246A_4131_BF7A_FE5CD641A19B
#define CF3E6AC4_246A_4131_BF7A_FE5CD641A19B

#include <libfork/core/impl/frame.hpp>

namespace lf::impl {

// -------------------------------------------------------- //

/**
 * @brief Awaitable in the context of an `lf::task` coroutine.
 */
struct [[nodiscard("A quasi_awaitable MUST be immediately co_awaited!")]] quasi_awaitable {
  frame *frame; ///< The parent/semaphore needs to be set!
};

// -------------------------------------------------------- //

} // namespace lf::impl

#endif /* CF3E6AC4_246A_4131_BF7A_FE5CD641A19B */
