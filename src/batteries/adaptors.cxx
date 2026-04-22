export module libfork.batteries:adaptors;

import std;

import libfork.core;

import :deque;

namespace lf {

export class adapt_vector {
 public:
  constexpr void push(unsafe_steal_handle value) { m_vector.push_back(value); }

  constexpr auto pop() noexcept -> unsafe_steal_handle {
    if (!m_vector.empty()) {
      unsafe_steal_handle value = m_vector.back();
      m_vector.pop_back();
      return value;
    }
    return {};
  }

 private:
  std::vector<unsafe_steal_handle> m_vector;
};

export class adapt_deque {
 public:
  constexpr void push(unsafe_steal_handle value) { m_deque.push(value); }

  constexpr auto pop() noexcept -> unsafe_steal_handle {
    return m_deque.pop([] static noexcept -> unsafe_steal_handle {
      return {};
    });
  }

  [[nodiscard]]
  constexpr auto thief() noexcept {
    return m_deque.thief();
  }

  [[nodiscard]]
  constexpr auto empty() const noexcept -> bool {
    return m_deque.empty();
  }

 private:
  deque<unsafe_steal_handle> m_deque{1024 * 32};
};

} // namespace lf
