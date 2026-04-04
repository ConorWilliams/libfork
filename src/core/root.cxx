module;
#include "libfork/__impl/assume.hpp"
export module libfork.core:root;

import std;

namespace lf {

// TODO: allocator aware!

struct root_task {
  struct promise_type {

    constexpr auto get_return_object() noexcept -> root_task { return {}; }

    constexpr static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

    constexpr static auto final_suspend() noexcept -> std::suspend_always { return {}; }

    [[noreturn]]
    constexpr void unhandled_exception() noexcept {
      LF_ASSUME(false);
    }
  };

  promise_type *m_promise;
};

} // namespace lf
