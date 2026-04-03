module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.core:geometric_stack;

import std;

import :constants;
import :utility;

namespace lf::stack {

/**
 * @brief A geometric_stack is a user-space (geometric) segmented program stack.
 *
 * This protects against hot-splitting by keeping a single cached segment.
 */
export template <typename Allocator = std::allocator<std::byte>>
class geometric {

  struct ctrl;
  struct node;

  // TODO: renames

  using ctrl_traits = std::allocator_traits<Allocator>::template rebind_traits<ctrl>;
  using node_traits = std::allocator_traits<Allocator>::template rebind_traits<node>;

  using ctrl_ptr = typename ctrl_traits::pointer;
  using node_ptr = typename node_traits::pointer;

  struct release_t {
    explicit constexpr release_t(key_t) noexcept {}
  };

  // TODO: use move's on all fancy pointer types

  class checkpoint_t {
   public:
    constexpr checkpoint_t() noexcept = default;
    constexpr auto operator==(checkpoint_t const &) const noexcept -> bool = default;

   private:
    friend geometric;
    explicit constexpr checkpoint_t(ctrl_ptr ptr) noexcept : m_ctrl(ptr) {}
    ctrl_ptr m_ctrl = nullptr;
  };

 public:
  constexpr geometric() noexcept(noexcept(Allocator{})) : geometric(Allocator()) {}
  explicit constexpr geometric(Allocator const &alloc) : m_heap_alloc(alloc), m_node_alloc(alloc) {}

  constexpr geometric(geometric const &other) = delete;
  constexpr geometric(geometric &&other) = delete;

  constexpr auto operator=(geometric const &other) -> geometric & = delete;
  constexpr auto operator=(geometric &&other) -> geometric & = delete;

  constexpr ~geometric() noexcept { delete_ctrl(); }

  /**
   * @brief Test if the stack is empty (all pushes have been popped).
   */
  [[nodiscard]]
  constexpr auto empty() const noexcept -> bool {
    if (m_ctrl == nullptr) {
      return true;
    }

    LF_ASSUME(m_ctrl->top != nullptr);

    if (m_ctrl->top->prev != nullptr) {
      return false;
    }
    return m_sp == m_lo;
  }

  [[nodiscard]]
  constexpr auto checkpoint() noexcept -> checkpoint_t {
    return checkpoint_t{m_ctrl};
  }

  [[nodiscard]]
  constexpr auto push(std::size_t size) -> void * {
    LF_ASSUME(size != 0);

    // Round such that next allocation is aligned.
    std::size_t padded_size = round_to_multiple<k_new_align>(size);

    if (padded_size > safe_cast<std::size_t>(m_hi - m_sp)) [[unlikely]] {
      return push_cached(padded_size);
    }

    LF_ASSUME(m_ctrl != nullptr);
    LF_ASSUME(m_ctrl->top != nullptr);

    return std::exchange(m_sp, m_sp + padded_size);
  }

  constexpr void pop(void *ptr, [[maybe_unused]] std::size_t n) noexcept {

    LF_ASSUME(m_ctrl != nullptr);
    LF_ASSUME(m_ctrl->top != nullptr);

    if (m_sp == m_lo) [[unlikely]] {
      pop_shuffle();
    }

    m_sp = static_cast<std::byte *>(ptr);
  }

  [[nodiscard]]
  constexpr auto prepare_release() const noexcept -> release_t {
    m_ctrl->sp_cache = m_sp;
    return release_t{key()};
  }

  constexpr void release([[maybe_unused]] release_t) noexcept {
    // Don't delete, will be resumed
    m_ctrl = nullptr;

    m_lo = nullptr;
    m_sp = nullptr;
    m_hi = nullptr;
  }

  constexpr void acquire(checkpoint_t ckpt) noexcept {

    LF_ASSUME(empty());
    LF_ASSUME(ckpt.m_ctrl != m_ctrl);

    if (ckpt.m_ctrl == nullptr) {
      return;
    }

    delete_ctrl();

    m_ctrl = ckpt.m_ctrl;

    LF_ASSUME(m_ctrl->top != nullptr);

    load_local<from::cache>();
  }

 private:
  // ============== Types ==============  //

  struct alignas(k_new_align) node {
    node_ptr prev;    // Linked list (past)
    std::size_t size; // Usable-size of the stacklet
  };

  struct ctrl {
    node_ptr top = nullptr;        // Most recent stacklet i.e. the top of the stack.
    node_ptr cache = nullptr;      // Cached (empty) stacklet for hot-split guarding.
    std::byte *sp_cache = nullptr; // Cached stack pointer for this stacklet.
  };

  // ============== Members ==============  //

  [[no_unique_address]]
  typename ctrl_traits::allocator_type m_heap_alloc;
  [[no_unique_address]]
  typename node_traits::allocator_type m_node_alloc;

  // TODO: rename root->heap

  ctrl_ptr m_ctrl = nullptr; // The control block for the stack.

  // TODO: use ptr

  std::byte *m_lo = nullptr; // The base pointer for the current stacklet.
  std::byte *m_sp = nullptr; // The stack pointer for the current stacklet.
  std::byte *m_hi = nullptr; // The one-past-the-end pointer for the current stacklet.

  // ============== Methods ==============  //

  /**
   * @brief Clean and delete the control block and all stacklets.
   */
  constexpr void delete_ctrl() noexcept {
    if (m_ctrl != nullptr) {
      LF_ASSUME(empty());
      LF_ASSUME(m_ctrl->top != nullptr);
      LF_ASSUME(m_ctrl->top->prev == nullptr);

      // Clea-up stacklets
      delete_node(m_ctrl->top);
      delete_node(m_ctrl->cache);

      // Finally delete the control block.
      ctrl_traits::destroy(m_heap_alloc, m_ctrl);
      ctrl_traits::deallocate(m_heap_alloc, m_ctrl, 1);
    }
  }

  enum class from : char {
    top,
    cache,
  };

  /**
   * @brief Make local pointers point to the current stacklet in the control block.
   *
   * Assumes that the control block and top stacklet are non-nullptr.
   */
  template <from From>
  constexpr auto load_local() noexcept -> void {
    LF_ASSUME(m_ctrl != nullptr);
    LF_ASSUME(m_ctrl->top != nullptr);

    m_lo = std::bit_cast<std::byte *>(m_ctrl->top + 1);

    if constexpr (From == from::cache) {
      m_sp = m_ctrl->sp_cache;
    } else {
      m_sp = m_lo;
    }

    m_hi = m_lo + m_ctrl->top->size;
  }

  // TODO: highlight local modifications with explicit self param
  // TODO: vet constexpr usage in the library

  /**
   * @brief Allocate node with size bytes for stacklet.
   *
   * This function is strongly exception-safe.
   */
  [[nodiscard]]
  constexpr auto new_node(this geometric &self, std::size_t size) -> node_ptr {

    // Allocation should be a multiple of the node size
    LF_ASSUME(size % sizeof(node) == 0);

    std::size_t num_nodes = 1 + (size / sizeof(node));

    node_ptr next_node = node_traits::allocate(self.m_node_alloc, num_nodes);

    LF_TRY {
      node_traits::construct(self.m_node_alloc, next_node, nullptr, size);
    } LF_CATCH_ALL {
      node_traits::deallocate(self.m_node_alloc, next_node, num_nodes);
      LF_RETHROW;
    }

    return next_node;
  }

  /**
   * @brief Delete a (possibly null) node and it's associated stacklet.
   */
  constexpr auto delete_node(this geometric &self, node_ptr ptr) noexcept -> void {
    if (ptr != nullptr) {
      std::size_t num_nodes = 1 + (ptr->size / sizeof(node));
      node_traits::destroy(self.m_node_alloc, ptr);
      node_traits::deallocate(self.m_node_alloc, ptr, num_nodes);
    }
  }

  // TODO: do we need the no inlines

  [[nodiscard]]
  LF_NO_INLINE constexpr auto push_cached(std::size_t padded_size) -> void *;

  constexpr void pop_shuffle() noexcept;
};

template <typename Allocator>
LF_NO_INLINE constexpr auto geometric<Allocator>::push_cached(std::size_t padded_size) -> void * {

  // Have to be very careful in this function to be strongly exception-safe!

  // This is the minimum size of node we could allocate that would fit the allocation.
  std::size_t min_node_size = padded_size + k_new_align - 1;

  if (m_ctrl == nullptr) {
    // Fine if this throw
    ctrl_ptr new_root = ctrl_traits::allocate(m_heap_alloc, 1);

    LF_TRY {
      ctrl_traits::construct(m_heap_alloc, new_root);
      LF_TRY {
        new_root->top = new_node(round_to_multiple<k_page_size>(min_node_size));
      } LF_CATCH_ALL {
        // Clean up construction
        ctrl_traits::destroy(m_heap_alloc, new_root);
        LF_RETHROW;
      }
    } LF_CATCH_ALL {
      // Clean up allocation
      ctrl_traits::deallocate(m_heap_alloc, new_root, 1);
      LF_RETHROW;
    }

    // Nothing can throw, safe to publish to *this.
    m_ctrl = new_root;

    // Local copies of the new top.
    load_local<from::top>();
    // Do the allocation.
    return std::exchange(m_sp, m_sp + padded_size);
  }

  LF_ASSUME(m_ctrl->top != nullptr);

  if (m_ctrl->cache != nullptr && m_ctrl->cache->size >= padded_size) {

    // We have space in the cache. No allocations on this path, nothing cam throw.

    if (m_sp == m_lo) {
      // There is nothing allocated on the current stacklet/top but it doesn't
      // have enough space hence, we need to delete top such that we don't end up
      // with an empty stacklet in the chain. This would break deletion otherwise.
      node_ptr empty_top = m_ctrl->top;
      m_ctrl->top = m_ctrl->top->prev; // top could be null now
      delete_node(empty_top);
    }

    // Shuffle cache to the top.
    m_ctrl->cache->prev = m_ctrl->top;
    m_ctrl->top = m_ctrl->cache;
    m_ctrl->cache = nullptr;

    // Local copies of the new top
    load_local<from::top>();
    // Do the allocation.
    return std::exchange(m_sp, m_sp + padded_size);
  }

  // We need to allocate a new stacklet to fit this allocation, we choose to
  // grow geometrically to try to avoid too many allocations.
  std::size_t next_node_size = std::max(min_node_size, 2 * m_ctrl->top->size);

  // Fine if this throws
  node_ptr new_top = new_node(round_to_multiple<k_page_size>(next_node_size));

  // Nothing can throw after this point

  // We didn't use the cache because it wasn't big enough, we should delete it
  // now because we had to grow the stack. We couldn't do this until now because
  // new_node may have thrown.
  delete_node(std::exchange(m_ctrl->cache, nullptr));

  if (m_sp == m_lo) {
    // There is nothing allocated on the current stacklet/top but it doesn't
    // have enough space hence, we need to delete top such that we don't end up
    // with an empty stacklet in the chain. This would break deletion otherwise.
    node_ptr empty_top = m_ctrl->top;
    m_ctrl->top = m_ctrl->top->prev; // top could be null now
    delete_node(empty_top);
  }

  // Commit the new/node
  new_top->prev = m_ctrl->top;
  m_ctrl->top = new_top;

  // Local copies of the new top
  load_local<from::top>();
  // Do the allocation.
  return std::exchange(m_sp, m_sp + padded_size);
}

template <typename Allocator>
constexpr void geometric<Allocator>::pop_shuffle() noexcept {
  // Shuffle top/cache
  LF_ASSUME(m_ctrl != nullptr);
  LF_ASSUME(m_ctrl->top != nullptr);       // Pop from empty stack
  LF_ASSUME(m_ctrl->top->prev != nullptr); // ^

  delete std::exchange(m_ctrl->cache, m_ctrl->top);
  m_ctrl->top = m_ctrl->top->prev;

  // Local copies of the new top
  load_local<from::top>();
}

} // namespace lf::stack
