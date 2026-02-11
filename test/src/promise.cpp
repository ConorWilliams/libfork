#include <catch2/catch_test_macros.hpp>

import libfork.core;

namespace {

struct dummy_allocator {

  struct ckpt {};

  constexpr static auto push(std::size_t sz) -> void *;
  constexpr static auto pop(void *p, std::size_t sz) noexcept -> void;
  constexpr static auto checkpoint() noexcept -> ckpt;
  constexpr static auto release() noexcept -> void;
  constexpr static auto acquire(ckpt) noexcept -> void;
};

static_assert(lf::stack_allocator<dummy_allocator>);

struct dummy_context {
  auto alloc() noexcept -> dummy_allocator &;
  void push(lf::frame_handle<dummy_context>);
  auto pop() noexcept -> lf::frame_handle<dummy_context>;
};

static_assert(lf::worker_context<dummy_context>);

} // namespace

TEST_CASE("Promise test", "[promise]") {

  using frame_t = lf::frame_type<dummy_context>;

  // Check for safe reinterpret_casts
  STATIC_CHECK(std::is_standard_layout_v<lf::frame_type<dummy_context>>);

  // Check on void
  static_assert(alignof(lf::promise_type<void, dummy_context>) == alignof(frame_t));

#ifdef __cpp_lib_is_pointer_interconvertible
  static_assert(std::is_pointer_interconvertible_with_class(&lf::promise_type<void, dummy_context>::frame));
#else
  static_assert(std::is_standard_layout_v<lf::promise_type<void, dummy_context>>);
#endif

  // Check on non-void
  static_assert(alignof(lf::promise_type<int, dummy_context>) == alignof(frame_t));

#ifdef __cpp_lib_is_pointer_interconvertible
  static_assert(std::is_pointer_interconvertible_with_class(&lf::promise_type<int, dummy_context>::frame));
#else
  static_assert(std::is_standard_layout_v<lf::promise_type<int, dummy_context>>);
#endif
}
