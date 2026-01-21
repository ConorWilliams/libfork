module;
#include <version>
export module libfork.core:promise;

import std;

namespace lf {

export template <typename T>
struct promise {
  auto test() -> std::string_view { return "hi"; }
};

struct basic_frame {
  basic_frame *parent;
};

template <typename T>
struct basic_promise;

template <>
struct basic_promise<void> {
  basic_frame frame;
};

template <typename T>
struct basic_promise {
  basic_frame frame;
  T *return_address;
};

static_assert(alignof(basic_promise<int>) == alignof(basic_promise<void>));

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_with_class(&basic_promise<void>::frame));
static_assert(std::is_pointer_interconvertible_with_class(&basic_promise<long>::frame));
#endif

} // namespace lf
