export module libfork.batteries:adaptors;

import std;

import libfork.core;

import :deque;

namespace lf {

export class adapt_vector {
 public:
  constexpr void push(handle value) { m_vector.push_back(value); }

  constexpr auto pop() noexcept -> handle {
    if (!m_vector.empty()) {
      handle value = m_vector.back();
      m_vector.pop_back();
      return value;
    }
    return {};
  }

 private:
  std::vector<handle> m_vector;
};

export class adapt_deque {
 public:
  constexpr void push(handle value) { m_deque.push(value); }

  constexpr auto pop() noexcept -> handle {
    return m_deque.pop([] static noexcept -> handle {
      return {};
    });
  }

  [[nodiscard]]
  constexpr auto thief() noexcept { return m_deque.thief(); }

  [[nodiscard]]
  constexpr auto empty() const noexcept -> bool { return m_deque.empty(); }

 private:
  deque<handle> m_deque{1024 * 32};
};

} // namespace lf
