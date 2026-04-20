module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.batteries:slab_stack;

import std;

import libfork.utils;

namespace lf {

/**
 * @brief A slab_stack is a user-space stack backed by a single fixed-size slab of memory.
 *
 * The ctrl metadata and usable stack space are fused into a single allocation: the first
 * node of the slab is the header, and the remaining `size` nodes are the usable stack
 * space.  There is no segmentation, caching, or geometric growth — if the slab is full,
 * push throws.
 *
 * For this to conform to `worker_stack` the allocators void pointer type must be `void *`
 */
export template <allocator_of<std::byte> Allocator = std::allocator<std::byte>>
class slab_stack {

  struct node; // Forward declaration so type aliases can reference node before its definition.

  using node_traits = std::allocator_traits<Allocator>::template rebind_traits<node>;
  using node_alloc_t = node_traits::allocator_type;
  using node_ptr = node_traits::pointer;
  using void_ptr = node_traits::void_pointer;
  using size_type = node_traits::size_type;
  using diff_type = node_traits::difference_type;

  // Fused ctrl+node: the first element of every slab allocation.
  // node_alloc, sp_cache, and size live here; the `size` nodes that follow are
  // the usable stack space.  Mirrors how geometric_stack stores ctrl data in its
  // first node — no reinterpret_cast is ever needed.
  struct alignas(k_new_align) node {
    [[no_unique_address]]
    node_alloc_t node_alloc; // Propagated to new owners on acquire.
    node_ptr sp_cache;       // Stack pointer saved across release/acquire.
    diff_type size;          // Usable node count following this header.
  };

  // Default capacity: one page of usable space (header occupies the first node).
  static constexpr diff_type k_default_nodes = safe_cast<diff_type>(k_page_size / sizeof(node)) - 1;

  static_assert(k_default_nodes > 0);

  struct release_t {
    explicit constexpr release_t(key_t) noexcept {}
  };

  class checkpoint_t {
   public:
    constexpr checkpoint_t() noexcept = default;
    constexpr auto operator==(checkpoint_t const &) const noexcept -> bool = default;

   private:
    friend slab_stack;
    explicit constexpr checkpoint_t(node_ptr ptr) noexcept : m_ctrl(ptr) {}
    node_ptr m_ctrl = nullptr;
  };

 public:
  constexpr slab_stack() : slab_stack(Allocator{}) {}
  explicit constexpr slab_stack(diff_type num_nodes) : slab_stack(Allocator{}, num_nodes) {}
  explicit constexpr slab_stack(Allocator const &alloc, diff_type num_nodes = k_default_nodes)
      : m_alloc(alloc) {
    init_slab(num_nodes);
  }

  constexpr slab_stack(slab_stack const &) = delete;
  constexpr slab_stack(slab_stack &&) = delete;

  constexpr auto operator=(slab_stack const &) -> slab_stack & = delete;
  constexpr auto operator=(slab_stack &&) -> slab_stack & = delete;

  constexpr ~slab_stack() noexcept {
    LF_ASSUME(empty());
    delete_ctrl(m_ctrl);
  }

  /**
   * @brief Test if the stack is empty (all pushes have been popped).
   */
  [[nodiscard]]
  constexpr auto empty() const noexcept -> bool {
    return m_sp == m_lo;
  }

  /**
   * @brief Get a checkpoint of the stack for transfer to another stack instance.
   */
  [[nodiscard]]
  constexpr auto checkpoint() noexcept -> checkpoint_t {
    return checkpoint_t{m_ctrl};
  }

  /**
   * @brief Allocate size bytes on the stack and return a pointer to the base of the allocation.
   */
  [[nodiscard]]
  constexpr auto push(std::size_t size) -> void_ptr {
    LF_ASSUME(size > 0);

    constexpr diff_type node_size = sizeof(node);

    diff_type push_bytes = safe_cast<diff_type>(round_to_multiple<sizeof(node)>(size));

    LF_ASSUME(push_bytes >= node_size);
    LF_ASSUME(push_bytes % node_size == 0);

    // Optimized to just the subtraction because multiplication cancels the implicit division.
    diff_type free_bytes = node_size * (m_hi - m_sp);

    if (push_bytes > free_bytes) [[unlikely]] {
      LF_THROW(std::bad_alloc{});
    }

    diff_type num_nodes = push_bytes / node_size;

    // node_ptr -> void_ptr
    return static_cast<void_ptr>(std::exchange(m_sp, m_sp + num_nodes));
  }

  /**
   * @brief Deallocate the most recent allocation of n bytes at ptr.
   */
  constexpr void pop(void_ptr ptr, [[maybe_unused]] std::size_t n) noexcept {
    LF_ASSUME(!empty());
    LF_ASSUME(m_sp != nullptr);
    LF_ASSUME(ptr != nullptr);

    // Inverse of push: void_ptr -> node_ptr
    m_sp = static_cast<node_ptr>(ptr);
  }

  [[nodiscard]]
  constexpr auto prepare_release() noexcept -> release_t {
    // Guard against null ctrl (failed prior allocation in release()).
    if (m_ctrl != nullptr) {
      m_ctrl->sp_cache = m_sp;
    }
    return release_t{key()};
  }

  constexpr void release([[maybe_unused]] release_t) noexcept {

    diff_type next_size = (m_ctrl != nullptr) ? m_ctrl->size : k_default_nodes;

    // Hand off the current slab to whoever holds the checkpoint; clear local state.
    m_ctrl = nullptr;
    m_lo = nullptr;
    m_sp = nullptr;
    m_hi = nullptr;

    // Pre-allocate a fresh slab for our next use.

    LF_TRY {
      init_slab(next_size);
      // If this throws, swallow the exception — push will see no space
      // i.e. (m_hi - m_sp == 0) and throw instead.
    } LF_CATCH_ALL {
    }
  }

  constexpr void acquire(checkpoint_t ckpt) noexcept {
    LF_ASSUME(empty());
    LF_ASSUME(ckpt.m_ctrl != m_ctrl);

    if (ckpt.m_ctrl == nullptr) {
      return;
    }

    // Discard the fresh empty slab we prepared during release() (may be null on alloc failure).
    delete_ctrl(m_ctrl);

    m_ctrl = ckpt.m_ctrl;

    if constexpr (!node_traits::is_always_equal::value) {
      m_alloc = node_alloc_t{std::as_const(m_ctrl->node_alloc)};
    }

    LF_ASSUME(m_ctrl != nullptr);

    load_local();
  }

 private:
  [[no_unique_address]]
  node_alloc_t m_alloc;

  node_ptr m_ctrl = nullptr; // Header node (fused ctrl+first-node of the slab).
  node_ptr m_lo = nullptr;   // Base of usable space (m_ctrl + 1).
  node_ptr m_sp = nullptr;   // Stack pointer for the current slab.
  node_ptr m_hi = nullptr;   // One-past-the-end of usable space.

  // Restore local pointers from the header node, taking sp from the cache.
  constexpr void load_local() noexcept {
    LF_ASSUME(m_ctrl != nullptr);
    m_lo = m_ctrl + 1;
    m_hi = m_lo + m_ctrl->size;
    m_sp = m_ctrl->sp_cache;
  }

  // Allocate and construct a fresh slab with num_nodes usable nodes.
  constexpr void init_slab(diff_type num_nodes) {
    LF_ASSUME(num_nodes > 0);

    size_type total = safe_cast<size_type>(1 + num_nodes);
    m_ctrl = node_traits::allocate(m_alloc, total);

    LF_TRY {
      node_traits::construct(m_alloc, std::to_address(m_ctrl), m_alloc, nullptr, num_nodes);
    } LF_CATCH_ALL {
      node_traits::deallocate(m_alloc, m_ctrl, total);
      m_ctrl = nullptr;
      LF_RETHROW;
    }

    m_lo = m_sp = m_ctrl + 1;
    m_hi = m_lo + num_nodes;
  }

  // Destroy and deallocate a slab (no-op if null).
  constexpr void delete_ctrl(node_ptr ctrl) noexcept {
    if (ctrl != nullptr) {
      size_type total = safe_cast<size_type>(1 + ctrl->size);
      node_traits::destroy(m_alloc, std::to_address(ctrl));
      node_traits::deallocate(m_alloc, ctrl, total);
    }
  }
};

} // namespace lf
