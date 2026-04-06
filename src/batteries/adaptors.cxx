export module libfork.batteries:adaptors;

import std;

import libfork.core;

import :deque;

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

export template <typename Context>
class adapt_deque {
 public:
  constexpr void push(steal_handle<Context> value) { m_deque.push(value); }

  constexpr auto pop() noexcept -> steal_handle<Context> {
    return m_deque.pop([] static noexcept -> steal_handle<Context> {
      return {};
    });
  }

 private:
  deque<steal_handle<Context>> m_deque{1024};
};

} // namespace lf
