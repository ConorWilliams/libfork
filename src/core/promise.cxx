export module libfork.core:promise;

import std;

namespace lf {

export template <typename T>
struct promise {
  auto test() -> std::string_view { return "hi"; }
};

} // namespace lf
