module;
#include <version>

#include "libfork/macros.hpp"
export module libfork.core:promise;

import std;

namespace lf {

// =============== Frame =============== //

struct frame_type {
  frame_type *parent;
};

static_assert(std::is_standard_layout_v<frame_type>);

// =============== Frame-mixin =============== //

struct mixin_frame {
  auto self(this auto &&self) LF_HOF(LF_FWD(self).frame)
};

static_assert(std::is_empty_v<mixin_frame>);

// =============== Promises =============== //

template <typename T>
struct promise_type;

template <>
struct promise_type<void> : mixin_frame {
  frame_type frame;
};

static_assert(alignof(promise_type<void>) == alignof(frame_type));

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_with_class(&promise_type<void>::frame));
#else
static_assert(std::is_standard_layout_v<promise_type<void>>);
#endif

template <typename T>
struct promise_type : mixin_frame {
  frame_type frame;
  T *return_address;
};

static_assert(alignof(promise_type<int>) == alignof(frame_type));

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_with_class(&promise_type<int>::frame));
#else
static_assert(std::is_standard_layout_v<promise_type<int>>);
#endif

} // namespace lf
