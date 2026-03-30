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

  struct key {
    friend geometric_stack;

   private:
    constexpr key() = default;
  };

  class checkpoint_t {
   public:
    auto operator==(checkpoint_t const &) const noexcept -> bool = default;

    constexpr checkpoint_t() = default; // Required to be regular

   private:
    explicit constexpr checkpoint_t(heap *root) noexcept : m_root(root) {
      LF_ASSUME(root != nullptr); //
    }
    friend class geometric_stack;
    heap *m_root;
  };

  [[nodiscard]]
  LF_NO_INLINE constexpr auto push_cached(std::size_t padded_size) -> void *;

  [[nodiscard]]
  constexpr auto push_alloc(std::size_t padded_size) -> void *;

  constexpr void pop_shuffle() noexcept;

 public:
  [[nodiscard]]
  constexpr auto checkpoint() noexcept -> checkpoint_t {

    // TODO: revisit if this + exception is worth is for no-alloc recoverability.

    // if (!m_root) [[unlikely]] {
    //   m_root.reset(new heap);
    // }

    return checkpoint_t{m_root.get()};
  }

  [[nodiscard]]
  constexpr auto push(std::size_t size) -> void * {
    // Round such that next allocation is aligned.
    std::size_t padded_size = round_to_multiple<k_new_align>(size);

    if (padded_size > safe_cast<std::size_t>(m_hi - m_sp)) [[unlikely]] {
      return push_cached(padded_size);
    }
    return std::exchange(m_sp, m_sp + padded_size);
  }

  constexpr void pop(void *ptr, [[maybe_unused]] std::size_t n) noexcept {
    if (m_sp == m_lo) [[unlikely]] {
      pop_shuffle();
    }
    m_sp = static_cast<std::byte *>(ptr);
  }

  constexpr auto prepare_release() noexcept -> key {
    m_root->sp_cache = m_sp;
    return {};
  }

  // TODO: drop noexcept requirement in concept

  constexpr void release([[maybe_unused]] key) noexcept {

    // Potentially throwing so call before release
    heap *fresh_heap = new heap;

    std::ignore = m_root.release();

    // Prime with new heap so that .checkpoint is valid.
    m_root.reset(fresh_heap);
    m_lo = nullptr;
    m_sp = nullptr;
    m_hi = nullptr;
  }

  constexpr void acquire(checkpoint_t ckpt) noexcept {
    if (ckpt.m_root != m_root.get()) {

      m_root.reset(ckpt.m_root);

      if (m_root->top != nullptr) {
        m_lo = m_root->top->stacklet.get();
        m_sp = m_root->sp_cache;
        m_hi = m_lo + m_root->top->size;
      } else {
        m_lo = nullptr;
        m_sp = nullptr;
        m_hi = nullptr;
      }
    }
  }

 private:
  struct deleter {
    static constexpr void operator()(heap *h) noexcept {
      // Should be empty at destruction.
      LF_ASSUME(h->top == nullptr || h->top->prev == nullptr);
      delete h->cache;
      delete h->top;
      delete h;
    }
  };

  std::unique_ptr<heap, deleter> m_root{new heap}; // The control block, never null.

  std::byte *m_lo = nullptr; // The base pointer for the current stacklet.
  std::byte *m_sp = nullptr; // The stack pointer for the current stacklet.
  std::byte *m_hi = nullptr; // The one-past-the-end pointer for the current stacklet.
};

LF_NO_INLINE
constexpr auto geometric_stack::push_cached(std::size_t padded_size) -> void * {

  if (m_sp == m_lo && m_root->top != nullptr) {
    // There is nothing allocated on the current stacklet/top but it doesn't
    // have enough space hence, we need to delete top such that we don't end up
    // with an empty stacklet in the chain. This would break deletion otherwise.
    delete std::exchange(m_root->top, m_root->top->prev);
  }

  if (m_root->cache != nullptr) {
    if (m_root->cache->size >= padded_size) {
      // Set cache as the new top
      m_root->cache->prev = m_root->top;
      m_root->top = std::exchange(m_root->cache, nullptr);

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
  // Must fallback to allocation
  return push_alloc(padded_size);
}

constexpr auto geometric_stack::push_alloc(std::size_t padded_size) -> void * {

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
