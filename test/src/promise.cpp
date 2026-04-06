#include <catch2/catch_test_macros.hpp>

import libfork;


// TODO: make the tests part of the module so they can access the internals?

TEST_CASE("Promise test", "[promise]") {

  using ckpt_t = lf::checkpoint_t<lf::dummy_context>;
  using frame_t = lf::frame_type<ckpt_t>;

  // Check for safe reinterpret_casts
  STATIC_CHECK(std::is_standard_layout_v<frame_t>);

  // Check on void
  static_assert(alignof(lf::promise_type<void, lf::dummy_context>) == alignof(frame_t));

#ifdef __cpp_lib_is_pointer_interconvertible
  static_assert(
      std::is_pointer_interconvertible_with_class(&lf::promise_type<void, lf::dummy_context>::frame));
#else
  static_assert(std::is_standard_layout_v<lf::promise_type<void, lf::dummy_context>>);
#endif

  // Check on non-void
  static_assert(alignof(lf::promise_type<int, lf::dummy_context>) == alignof(frame_t));

#ifdef __cpp_lib_is_pointer_interconvertible
  static_assert(
      std::is_pointer_interconvertible_with_class(&lf::promise_type<int, lf::dummy_context>::frame));
#else
  static_assert(std::is_standard_layout_v<lf::promise_type<int, lf::dummy_context>>);
#endif
}
