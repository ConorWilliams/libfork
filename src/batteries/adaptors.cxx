export module libfork.batteries:adaptors;

import std;

import libfork.core;
import libfork.utils;

import :deque;

namespace lf {

export template <allocator_of<unsafe_steal_handle> Allocator = std::allocator<unsafe_steal_handle>>
class adapt_vector {
 public:
  constexpr adapt_vector() = default;

  explicit constexpr adapt_vector(Allocator const &alloc) noexcept : m_vector(alloc) {}

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
  std::vector<unsafe_steal_handle, Allocator> m_vector;
};

export template <allocator_of<std::atomic<unsafe_steal_handle>> Allocator =
                     std::allocator<std::atomic<unsafe_steal_handle>>>
class adapt_deque {
  using size_type = deque<unsafe_steal_handle, Allocator>::size_type;

  static constexpr size_type k_default_capacity = 1024 * 32;

 public:
  constexpr adapt_deque() : m_deque{k_default_capacity} {}

  explicit constexpr adapt_deque(size_type capacity, Allocator const &alloc = Allocator())
      : m_deque{capacity, alloc} {}

  constexpr void push(unsafe_steal_handle value) { m_deque.push(value); }

  constexpr auto pop() noexcept -> unsafe_steal_handle {
    return m_deque.pop([] static noexcept -> unsafe_steal_handle {
      return {};
    });
  }

  constexpr auto steal() noexcept -> unsafe_steal_handle {
    if (auto [_, result] = m_deque.thief().steal()) {
      return result;
    }
    return {};
  }

 private:
  deque<unsafe_steal_handle, Allocator> m_deque;
};

} // namespace lf
