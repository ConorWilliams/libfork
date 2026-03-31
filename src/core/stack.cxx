module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
export module libfork.core:stack;

import std;

import :constants;
import :utility;

namespace lf {

/**
 * @brief A geometric_stack is a user-space (geometric) segmented program stack.
 */
export class geometric_stack {

  struct node;

  struct node_data : immovable {
    node *prev;                                               // Linked list (past).
    std::size_t size;                                         // Size of stacklet.
    std::unique_ptr<std::byte[]> stacklet = make_bytes(size); // Actual data.
  };

  // Align such that the entire node is on a cache line
  struct alignas(std::max(std::bit_ceil(sizeof(node_data)), alignof(node_data))) node : node_data {
    constexpr node(node *prev_arg, std::size_t size_arg) : node_data{.prev = prev_arg, .size = size_arg} {
      // Each stacklet should be on a boundary.
      LF_ASSUME(is_aligned<k_new_align>(stacklet.get()));
    }
  };

  static_assert(sizeof(node) == alignof(node) && alignof(node) <= k_cache_line);

  struct heap : immovable {
    node *top = nullptr;           // Most recent stacklet i.e. the top of the stack.
    node *cache = nullptr;         // Cached (empty) stacklet for hot-split guarding.
    std::byte *sp_cache = nullptr; // Cached stack pointer for this stacklet.
  };

  struct release_key {
    friend geometric_stack;

   private:
    constexpr release_key() = default;
  };

  [[nodiscard]]
  LF_NO_INLINE constexpr auto push_cached(std::size_t padded_size) -> void *;

  [[nodiscard]]
  constexpr auto push_alloc(std::size_t padded_size) -> void *;

  constexpr void pop_shuffle() noexcept;

 public:
  [[nodiscard]]
  constexpr auto empty() noexcept -> bool {
    if (m_root == nullptr) {
      return true;
    }

    LF_ASSUME(m_root->top != nullptr);

    if (m_root->top->prev != nullptr) {
      return false;
    }
    return m_sp == m_lo;
  }

  [[nodiscard]]
  constexpr auto checkpoint() noexcept -> opaque {
    return {key(), m_root.get()};
  }

  [[nodiscard]]
  constexpr auto push(std::size_t size) -> void * {
    LF_ASSUME(size != 0);

    // Round such that next allocation is aligned.
    std::size_t padded_size = round_to_multiple<k_new_align>(size);

    if (padded_size > safe_cast<std::size_t>(m_hi - m_sp)) [[unlikely]] {
      return push_cached(padded_size);
    }

    LF_ASSUME(m_root != nullptr);
    LF_ASSUME(m_root->top != nullptr);

    return std::exchange(m_sp, m_sp + padded_size);
  }

  constexpr void pop(void *ptr, [[maybe_unused]] std::size_t n) noexcept {

    LF_ASSUME(m_root != nullptr);
    LF_ASSUME(m_root->top != nullptr);

    if (m_sp == m_lo) [[unlikely]] {
      pop_shuffle();
    }
    m_sp = static_cast<std::byte *>(ptr);
  }

  [[nodiscard]]
  constexpr auto prepare_release() const noexcept -> release_key {
    m_root->sp_cache = m_sp;
    return {};
  }

  constexpr void release([[maybe_unused]] release_key) noexcept {
    // Safe even if we are nullptr
    std::ignore = m_root.release();
    m_lo = nullptr;
    m_sp = nullptr;
    m_hi = nullptr;
  }

  constexpr void acquire(opaque ckpt) noexcept {

    heap *ckpt_root = ckpt.cast<heap>();

    LF_ASSUME(empty());
    LF_ASSUME(ckpt_root != m_root.get());

    if (ckpt_root == nullptr) {
      return;
    }

    m_root.reset(ckpt_root);

    LF_ASSUME(m_root->top != nullptr);
    m_lo = m_root->top->stacklet.get();
    m_sp = m_root->sp_cache;
    m_hi = m_lo + m_root->top->size;
  }

 private:
  struct deleter {
    static constexpr void operator()(heap *h) noexcept {
      // Should be empty at destruction.
      LF_ASSUME(h->top != nullptr);
      LF_ASSUME(h->top->prev == nullptr);
      delete h->cache;
      delete h->top;
      delete h;
    }
  };

  std::unique_ptr<heap, deleter> m_root; // The control block.

  std::byte *m_lo = nullptr; // The base pointer for the current stacklet.
  std::byte *m_sp = nullptr; // The stack pointer for the current stacklet.
  std::byte *m_hi = nullptr; // The one-past-the-end pointer for the current stacklet.
};

LF_NO_INLINE
constexpr auto geometric_stack::push_cached(std::size_t padded_size) -> void * {
  if (!m_root) {
    // Need to allocate a control block
    m_root.reset(new heap);
  } else {
    LF_ASSUME(m_root->top != nullptr);

    if (m_sp == m_lo) {
      // There is nothing allocated on the current stacklet/top but it doesn't
      // have enough space hence, we need to delete top such that we don't end up
      // with an empty stacklet in the chain. This would break deletion otherwise.
      delete std::exchange(m_root->top, m_root->top->prev);
    }

    if (m_root->cache != nullptr) {
      if (m_root->cache->size >= padded_size) {
        // We have space in the cache, shuffle it to the top.
        m_root->cache->prev = m_root->top;
        m_root->top = m_root->cache;
        m_root->cache = nullptr;

        // Local copies of the new top
        m_lo = m_root->top->stacklet.get();
        m_sp = m_lo;
        m_hi = m_lo + m_root->top->size;

        // Do the allocation.
        return std::exchange(m_sp, m_sp + padded_size);
      }
      // Cache is too small, free it.
      delete std::exchange(m_root->cache, nullptr);
    }
  }
  // Must fallback to allocation
  return push_alloc(padded_size);
}

constexpr auto geometric_stack::push_alloc(std::size_t padded_size) -> void * {

  LF_ASSUME(m_root != nullptr);

  constexpr std::size_t growth_factor = 2;

  std::size_t stacklet_size = std::max(padded_size, m_root->top ? growth_factor * m_root->top->size : 0);

  // Link a new top node into control block.
  m_root->top = new node(m_root->top, round_to_multiple<k_page_size>(stacklet_size));

  // Local copies of the new top.
  m_lo = m_root->top->stacklet.get();
  m_sp = m_lo;
  m_hi = m_lo + m_root->top->size;

  // Do the allocation.
  return std::exchange(m_sp, m_sp + padded_size);
}

constexpr void geometric_stack::pop_shuffle() noexcept {
  // Shuffle top/cache
  LF_ASSUME(m_root != nullptr);
  LF_ASSUME(m_root->top != nullptr);       // Pop from empty stack
  LF_ASSUME(m_root->top->prev != nullptr); // ^

  delete std::exchange(m_root->cache, m_root->top);
  m_root->top = m_root->top->prev;

  // Local copies of the new top
  m_lo = m_root->top->stacklet.get();
  m_sp = m_lo;
  m_hi = m_lo + m_root->top->size;
}

} // namespace lf
