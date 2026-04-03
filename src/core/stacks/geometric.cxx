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
 *
 * For this to conform to `stack_allocator` the allocators void pointer type must be `void *`
 */
export template <typename Allocator = std::allocator<std::byte>>
class geometric {

  struct ctrl;
  struct node;

  using ctrl_traits = std::allocator_traits<Allocator>::template rebind_traits<ctrl>;
  using node_traits = std::allocator_traits<Allocator>::template rebind_traits<node>;

  using ctrl_alloc = ctrl_traits::allocator_type;
  using node_alloc = node_traits::allocator_type;

  using ctrl_ptr = ctrl_traits::pointer;
  using node_ptr = node_traits::pointer;

  using void_ptr = node_traits::void_pointer;

  using size_int = node_traits::size_type;
  using diff_int = node_traits::difference_type;

  struct release_t {
    explicit constexpr release_t(key_t) noexcept {}
  };

  class checkpoint_t {

    // An empty replacement for allocators that are always equal.
    struct empty {
      constexpr empty() noexcept = default;
      explicit constexpr empty(ctrl_alloc const &) noexcept {}
      constexpr auto operator==(empty const &) const noexcept -> bool = default;
    };

    using allocator = std::conditional_t<ctrl_traits::is_always_equal::value, empty, ctrl_alloc>;

   public:
    constexpr checkpoint_t() noexcept(noexcept(allocator())) : m_alloc() {}
    constexpr auto operator==(checkpoint_t const &) const noexcept -> bool = default;

   private:
    friend geometric;

    ctrl_ptr m_ctrl = nullptr;
    [[no_unique_address]]
    allocator m_alloc;

    explicit constexpr checkpoint_t(geometric const &stack) noexcept
        : m_ctrl(stack.m_ctrl),
          m_alloc{stack.m_ctrl_alloc} {}
  };

 public:
  constexpr geometric() noexcept(noexcept(Allocator{})) : geometric(Allocator()) {}
  explicit constexpr geometric(Allocator const &alloc) : m_ctrl_alloc(alloc) {}

  constexpr geometric(geometric const &other) = delete;
  constexpr geometric(geometric &&other) = delete;

  constexpr auto operator=(geometric const &other) -> geometric & = delete;
  constexpr auto operator=(geometric &&other) -> geometric & = delete;

  constexpr ~geometric() noexcept {
    LF_ASSUME(empty());
    delete_ctrl(m_ctrl);
  }

  /**
   * @brief Test if the stack is empty (all pushes have been popped).
   */
  [[nodiscard]]
  constexpr auto empty() const noexcept -> bool {

    if (m_ctrl != nullptr) {
      LF_ASSUME(m_ctrl->top != nullptr);
    } else {
      return true;
    }

    if (m_ctrl->top->prev != nullptr) {
      return false;
    }

    return m_sp == m_lo;
  }

  /**
   * @brief Get a checkpoint of the stack that can be used to acquire it from another stack allocator.
   */
  [[nodiscard]]
  constexpr auto checkpoint() noexcept -> checkpoint_t {
    return checkpoint_t{*this};
  }

  /**
   * @brief Allocate size bytes on the stack and return a pointer to the base of the allocation.
   */
  [[nodiscard]]
  constexpr auto push(std::size_t size) -> void_ptr {
    // Zero sized pushed are an error
    LF_ASSUME(size > 0);

    // Very careful math to avoid superfluous instructions on this (very) hot path.
    diff_int push_bytes = safe_cast<diff_int>(round_to_multiple<sizeof(node)>(size));

    constexpr diff_int node_size = sizeof(node);

    LF_ASSUME(push_bytes >= node_size);
    LF_ASSUME(push_bytes % node_size == 0);

    // Optimized to just the subtrtaction because multiplication cancels the implicit division.
    diff_int free_bytes = node_size * (m_hi - m_sp);

    if (push_bytes > free_bytes) [[unlikely]] {
      return push_cached(push_bytes);
    }

    LF_ASSUME(m_ctrl != nullptr);
    LF_ASSUME(m_ctrl->top != nullptr);

    // Compiler should optimize this division away when it fuses it with the
    // implicit multiplication in the pointer arithmetic below.
    diff_int num_nodes = push_bytes / node_size;

    // node_ptr -> void_ptr
    return static_cast<void_ptr>(std::exchange(m_sp, m_sp + num_nodes));
  }

  /**
   * @brief Deallocate the most recent allocation of size bytes at ptr, which
   * must be the most recent allocation returned by push and not yet popped.
   */
  constexpr void pop(void_ptr ptr, [[maybe_unused]] std::size_t n) noexcept {

    LF_ASSUME(!empty());
    LF_ASSUME(m_ctrl != nullptr);
    LF_ASSUME(m_ctrl->top != nullptr);
    LF_ASSUME(m_sp != nullptr);
    LF_ASSUME(ptr != nullptr);

    // Inverse of push: void_ptr -> node_ptr
    auto sp = static_cast<node_ptr>(ptr);

    if (m_sp == m_lo) [[unlikely]] {
      return pop_shuffle(sp);
    }

    m_sp = sp;
  }

  [[nodiscard]]
  constexpr auto prepare_release() const noexcept -> release_t {

    // Guard against null release
    if (m_ctrl != nullptr) {
      m_ctrl->sp_cache = m_sp;
    }

    return release_t{key()};
  }

  constexpr void release([[maybe_unused]] release_t) noexcept {

    // Don't delete, will be resumed
    m_ctrl = nullptr;

    m_lo = nullptr;
    m_sp = nullptr;
    m_hi = nullptr;
  }

  constexpr void acquire(checkpoint_t const &ckpt) noexcept {

    LF_ASSUME(empty());
    LF_ASSUME(ckpt.m_ctrl != m_ctrl);

    if (ckpt.m_ctrl == nullptr) {
      return;
    }

    delete_ctrl(m_ctrl);

    m_ctrl = ckpt.m_ctrl;

    if constexpr (!ctrl_traits::is_always_equal::value) {
      m_ctrl_alloc = ckpt.m_alloc;
    }

    LF_ASSUME(m_ctrl->top != nullptr);

    load_local<from::cache>();
  }

 private:
  // ============== Types ==============  //

  enum class from : char {
    top,
    cache,
    none,
  };

  struct alignas(k_new_align) node {
    node_ptr prev; // Linked list (past)
    diff_int size; // Usable-size of the stacklet
  };

  struct ctrl {
    [[no_unique_address]]
    typename node_traits::allocator_type node_alloc;

    node_ptr top = nullptr;      // Most recent stacklet i.e. the top of the stack.
    node_ptr cache = nullptr;    // Cached (empty) stacklet for hot-split guarding.
    node_ptr sp_cache = nullptr; // Cached stack pointer for this stacklet.
  };

  // ============== Members ==============  //

  [[no_unique_address]]
  typename ctrl_traits::allocator_type m_ctrl_alloc;

  ctrl_ptr m_ctrl = nullptr; // The control block for the stack.

  node_ptr m_lo = nullptr; // The base pointer for the current stacklet.
  node_ptr m_sp = nullptr; // The stack pointer for the current stacklet.
  node_ptr m_hi = nullptr; // The one-past-the-end pointer for the current stacklet.

  // ============== Methods ==============  //

  /**
   * @brief Make local pointers point to the current stacklet in the control block.
   *
   * Assumes that the control block and top stacklet are non-nullptr.
   */
  template <from StackPtr>
  constexpr auto load_local() noexcept -> void {

    LF_ASSUME(m_ctrl != nullptr);
    LF_ASSUME(m_ctrl->top != nullptr);

    constexpr diff_int one{1};

    m_lo = m_ctrl->top + one;
    m_hi = m_lo + m_ctrl->top->size;

    if constexpr (StackPtr == from::cache) {
      m_sp = m_ctrl->sp_cache;
    } else if constexpr (StackPtr == from::top) {
      m_sp = m_lo;
    } else {
      static_assert(StackPtr == from::none);
    }
  }

  /**
   * @brief Allocate and construct a new control block with a single stacklet of size bytes.
   */
  [[nodiscard]]
  constexpr auto new_ctrl(this geometric &self, diff_int num_nodes) -> ctrl_ptr {

    ctrl_ptr new_ctrl = ctrl_traits::allocate(self.m_ctrl_alloc, 1);

    LF_TRY {
      // Propagate ctrl allocator to control blocks node allocator.
      ctrl_traits::construct(self.m_ctrl_alloc, new_ctrl, std::as_const(self.m_ctrl_alloc));
      LF_TRY {
        new_ctrl->top = new_node(new_ctrl, num_nodes);
      } LF_CATCH_ALL {
        // Clean up construction
        ctrl_traits::destroy(self.m_ctrl_alloc, new_ctrl);
        LF_RETHROW;
      }
    } LF_CATCH_ALL {
      // Clean up allocation
      ctrl_traits::deallocate(self.m_ctrl_alloc, new_ctrl, 1);
      LF_RETHROW;
    }

    return new_ctrl;
  }

  /**
   * @brief Clean and delete the control block and all stacklets.
   */
  constexpr void delete_ctrl(this geometric &self, ctrl_ptr ctrl) noexcept {
    if (ctrl != nullptr) {
      LF_ASSUME(ctrl->top != nullptr);
      LF_ASSUME(ctrl->top->prev == nullptr);

      // Clea-up stacklets
      delete_node(ctrl, ctrl->top);
      delete_node(ctrl, ctrl->cache);

      // Finally delete the control block.
      ctrl_traits::destroy(self.m_ctrl_alloc, ctrl);
      ctrl_traits::deallocate(self.m_ctrl_alloc, ctrl, 1);
    }
  }

  // TODO: vet constexpr usage in the library

  /**
   * @brief Allocate node with size bytes for stacklet.
   *
   * This function is strongly exception-safe.
   */
  [[nodiscard]]
  static constexpr auto new_node(ctrl_ptr ctrl, diff_int num_nodes) -> node_ptr {

    // Allocation should be a multiple of the node size
    LF_ASSUME(num_nodes > 0);
    LF_ASSUME(ctrl != nullptr);

    // Allocation/deallocation requires size_int, +1 for the header node
    size_int allocate_nodes = 1 + safe_cast<size_int>(num_nodes);

    node_ptr next_node = node_traits::allocate(ctrl->node_alloc, allocate_nodes);

    LF_TRY {
      // Construct the header
      node_traits::construct(ctrl->node_alloc, next_node, nullptr, num_nodes);
    } LF_CATCH_ALL {
      node_traits::deallocate(ctrl->node_alloc, next_node, allocate_nodes);
      LF_RETHROW;
    }

    return next_node;
  }

  /**
   * @brief Delete a (possibly null) node and it's associated stacklet.
   */
  static constexpr auto delete_node(ctrl_ptr ctrl, node_ptr ptr) noexcept -> void {
    if (ptr != nullptr) {
      LF_ASSUME(ctrl != nullptr);
      // Size doesn't include the header node so we +1 here.
      size_int allocated_nodes = safe_cast<size_int>(1 + ptr->size);
      node_traits::destroy(ctrl->node_alloc, ptr);
      node_traits::deallocate(ctrl->node_alloc, ptr, allocated_nodes);
    }
  }

  [[nodiscard]]
  constexpr auto push_cached(diff_int push_bytes) -> void_ptr {

    // Have to be very careful in this function to be strongly exception-safe!

    constexpr diff_int node_size = sizeof(node);

    LF_ASSUME(push_bytes >= node_size);
    LF_ASSUME(push_bytes % node_size == 0);

    diff_int num_nodes = safe_cast<diff_int>(push_bytes / node_size);

    LF_ASSUME(num_nodes > 0);

    if (m_ctrl == nullptr) {
      // Initial stacklet wants to be quite large
      constexpr diff_int min_nodes = (k_page_size / sizeof(node)) - 1;

      m_ctrl = new_ctrl(std::max(min_nodes, num_nodes));

      // Local copies of the new top
      load_local<from::top>();
      // Do the allocation.
      return static_cast<void_ptr>(std::exchange(m_sp, m_sp + num_nodes));
    }

    LF_ASSUME(m_ctrl->top != nullptr);

    if (m_ctrl->cache != nullptr && m_ctrl->cache->size >= num_nodes) {

      // We have space in the cache. No allocations on this path, nothing cam throw.

      if (m_sp == m_lo) {
        // There is nothing allocated on the current stacklet/top but it doesn't
        // have enough space hence, we need to delete top such that we don't end up
        // with an empty stacklet in the chain. This would break deletion otherwise.
        node_ptr empty_top = m_ctrl->top;
        m_ctrl->top = m_ctrl->top->prev; // top could be null now
        delete_node(m_ctrl, empty_top);
      }

      // Shuffle cache to the top.
      m_ctrl->cache->prev = m_ctrl->top;
      m_ctrl->top = m_ctrl->cache;
      m_ctrl->cache = nullptr;

      // Local copies of the new top
      load_local<from::top>();
      // Do the allocation.
      return static_cast<void_ptr>(std::exchange(m_sp, m_sp + num_nodes));
    }

    // We need to allocate a new stacklet to fit this allocation, we choose to
    // grow geometrically to try to avoid too many allocations. Fine if this
    // throws
    node_ptr new_top = new_node(m_ctrl, std::max(num_nodes, 2 * m_ctrl->top->size));

    // Nothing can throw after this point

    // We didn't use the cache because it wasn't big enough, we should delete it
    // now because we had to grow the stack. We couldn't do this until now because
    // new_node may have thrown.
    delete_node(m_ctrl, std::exchange(m_ctrl->cache, nullptr));

    if (m_sp == m_lo) {
      // There is nothing allocated on the current stacklet/top but it doesn't
      // have enough space hence, we need to delete top such that we don't end up
      // with an empty stacklet in the chain. This would break deletion otherwise.
      node_ptr empty_top = m_ctrl->top;
      m_ctrl->top = m_ctrl->top->prev; // top could be null now
      delete_node(m_ctrl, empty_top);
    }

    // Commit the new/node
    new_top->prev = m_ctrl->top;
    m_ctrl->top = new_top;

    // Local copies of the new top
    load_local<from::top>();
    // Do the allocation.
    return static_cast<void_ptr>(std::exchange(m_sp, m_sp + num_nodes));
  }

  constexpr void pop_shuffle(node_ptr sp) noexcept {

    // Shuffle top/cache
    LF_ASSUME(!empty());
    LF_ASSUME(m_ctrl != nullptr);
    LF_ASSUME(m_ctrl->top != nullptr);       // Pop from empty stack
    LF_ASSUME(m_ctrl->top->prev != nullptr); // ^

    // Shuffle top to cache
    node_ptr old_cache = m_ctrl->cache;
    m_ctrl->cache = m_ctrl->top;
    delete_node(m_ctrl, old_cache);

    // Go back one stacklet
    m_ctrl->top = m_ctrl->top->prev;

    // Local copies of the new top
    load_local<from::none>();
    m_sp = sp;
  }
};

} // namespace lf::stack
