#ifndef BC495EA7_6A42_4FF0_96F0_0EAF9EBD672B
#define BC495EA7_6A42_4FF0_96F0_0EAF9EBD672B

#include "libfork/detail/utility.hpp"

namespace lf {

/**
 * @brief An enumeration that determines the behavior of a coroutine's promise.
 */
enum class tag {
  /**
   * @brief This coroutine is a root task from an ``lf::sync_wait``.
   */
  root,
  /**
   * @brief Non root task from an ``lf::call``, completes synchronously.
   */
  call,
  /**
   * @brief Non root task from an ``lf::fork``, completes asynchronously.
   */
  fork,
};

struct scope : detail::immovable<scope> {};

} // namespace lf

#endif /* BC495EA7_6A42_4FF0_96F0_0EAF9EBD672B */
