export module libfork.core:adaptors;

import :handles;

namespace lf {

// TODO: move stacks and contexts out of core and into substrate?

export template <typename Context>
class adapt_vector {
 public:
  constexpr void push(steal_handle<Context> value) { m_vector.push_back(value); }

  constexpr auto pop() noexcept -> steal_handle<Context> {
    if (!m_vector.empty()) {
      steal_handle<Context> value = m_vector.back();
      m_vector.pop_back();
      return value;
    }
    return {};
  }

 private:
  std::vector<steal_handle<Context>> m_vector;
};

} // namespace lf
