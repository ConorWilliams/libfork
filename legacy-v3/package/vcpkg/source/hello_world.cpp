#include <iostream>

#include "libfork/core.hpp"
#include "libfork/schedule/lazy_pool.hpp"

#ifndef LF_USE_HWLOC
static_assert(false, "libfork requires HWLOC support");
#endif

#include <hwloc.h>

namespace {

constexpr auto hello_async_world = [](auto /* self */) -> lf::task<int> {
  std::cout << "Hello, async world!" << std::endl;
  co_return 0;
};

} // namespace

auto main() -> int {
  try {
    return lf::sync_wait(lf::lazy_pool{}, hello_async_world);
  } catch (std::exception const &e) {
    std::cerr << "Caught exception: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Caught unknown exception." << std::endl;
    return 1;
  }
}