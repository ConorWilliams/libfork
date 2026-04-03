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
class geometric : immovable {

  struct ctrl;
  struct node;

  using heap_traits = std::allocator_traits<Allocator>::template rebind_traits<ctrl>;
  using node_traits = std::allocator_traits<Allocator>::template rebind_traits<node>;

  using heap_ptr = typename heap_traits::pointer;
  using node_ptr = typename node_traits::pointer;

  struct release_t {
    constexpr release_t(key_t) noexcept {}
  };

  // TODO: use move's on all fancy pointer types

  class checkpoint_t {
   public:
    constexpr checkpoint_t() noexcept = default;
    constexpr auto operator==(checkpoint_t const &) const noexcept -> bool = default;

   private:
    friend geometric;
    explicit constexpr checkpoint_t(heap_ptr ptr) noexcept : m_ptr(ptr) {}
    heap_ptr m_ptr = nullptr;
  };

 public:
  constexpr geometric() noexcept(noexcept(Allocator{})) : geometric(Allocator()) {}

  explicit constexpr geometric(Allocator const &alloc)
      : m_heap_alloc(alloc),
        m_node_alloc(alloc),
        m_byte_alloc(alloc) {}

  /**
   * @brief Test if the stack is empty (all pushes have been popped).
   */
  [[nodiscard]]
  constexpr auto empty() const noexcept -> bool {
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
  constexpr auto checkpoint() noexcept -> checkpoint_t {
    return checkpoint_t{m_root};
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
  constexpr auto prepare_release() const noexcept -> release_t {
    m_root->sp_cache = m_sp;
    return {};
  }

  constexpr void release([[maybe_unused]] release_t) noexcept {
    // Safe even if we are nullptr
    std::ignore = m_root.release();
    m_lo = nullptr;
    m_sp = nullptr;
    m_hi = nullptr;
  }

  constexpr void acquire(checkpoint_t ckpt) noexcept {

    LF_ASSUME(empty());
    LF_ASSUME(ckpt_root != m_root.get());

    if (ckpt_root == nullptr) {
      return;
    }

    m_root.reset(ckpt_root);

    LF_ASSUME(m_root->top != nullptr);

    // Not quite a load_local because sp = sp_cache
    m_lo = m_root->top->stacklet;
    m_sp = m_root->sp_cache;
    m_hi = m_lo + m_root->top->size;
  }

  constexpr ~geometric() noexcept {
    // TODO:

    // struct deleter {
    //   static constexpr void operator()(heap *h) noexcept {
    //     // Should be empty at destruction.
    //     LF_ASSUME(h->top != nullptr);
    //     LF_ASSUME(h->top->prev == nullptr);
    //     delete h->cache;
    //     delete h->top;
    //     delete h;
    //   }
    // };
  }

 private:
  // ============== Types ==============  //

  struct header {
    node_ptr prev;    // Linked list (past)
    std::size_t size; // Usable-size of the stacklet
  };

  struct alignas(k_new_align) node {
    union {
      std::byte _[sizeof(header)];
      header head;
    };
  };

  struct ctrl {
    node_ptr top = nullptr;        // Most recent stacklet i.e. the top of the stack.
    node_ptr cache = nullptr;      // Cached (empty) stacklet for hot-split guarding.
    std::byte *sp_cache = nullptr; // Cached stack pointer for this stacklet.
  };

  // ============== Members ==============  //

  [[no_unique_address]]
  typename heap_traits::allocator_type m_heap_alloc;
  [[no_unique_address]]
  typename node_traits::allocator_type m_node_alloc;

  heap_ptr m_root = nullptr; // The control block for the stack.

  node_ptr m_lo = nullptr; // The base pointer for the current stacklet.
  node_ptr m_sp = nullptr; // The stack pointer for the current stacklet.
  node_ptr m_hi = nullptr; // The one-past-the-end pointer for the current stacklet.

  // ============== Methods ==============  //

  // TODO: do we need the no inlines

  [[nodiscard]]
  LF_NO_INLINE constexpr auto push_cached(std::size_t padded_size) -> void *;

  constexpr void pop_shuffle() noexcept;

  /**
   * @brief Make local pointers point to the current stacklet in the control block.
   *
   * Assumes that the control block and top stacklet are non-nullptr.
   */
  constexpr auto load_local() noexcept -> void {
    LF_ASSUME(m_root != nullptr);
    LF_ASSUME(m_root->top != nullptr);
    LF_ASSUME(m_root->top->stacklet != nullptr);
    m_lo = m_root->top->stacklet;
    m_sp = m_lo;
    m_hi = m_lo + m_root->top->size;
  }

  // TODO: highlight local modifications with explicit self param

  /**
   * @brief Allocate node with size bytes for stacklet,
   *
   * Note the resultant stacklet may be smaller as it will be adjusted for
   * alignment. If you need `x` bytes allocate `x + k_new_align - 1` bytes.
   *
   * This function is strongly exception-safe.
   */
  [[nodiscard]]
  constexpr auto new_node(node_ptr prev, std::size_t size) -> node_ptr {
    // Don't need to construct (implicit lifetime bytes)
    byte_ptr byte_data = byte_traits::allocate(m_byte_alloc, size);
    node_ptr next_node = nullptr;

    LF_TRY {
      next_node = node_traits::allocate(m_node_alloc, 1);
    } LF_CATCH_ALL {
      byte_traits::deallocate(m_byte_alloc, byte_data, size);
      LF_RETHROW;
    }

    // From here on nothing can (is allowed to) throw.

    static_assert(std::has_single_bit(k_new_align), "Alignment is a power of two");

    std::byte *raw_stacklet = std::to_address(byte_data);

    // How much we need to increment to align:
    //
    //   (y - (x mod y)) mod y = (- x) mod y
    //                         = (- x) & (y - 1) as y is a power of two
    //
    std::size_t offset = (-std::bit_cast<std::size_t>(raw_stacklet)) & (k_new_align - 1);

    node next{
        .prev = prev,
        .raw_ptr = byte_data,
        .raw_size = size,
        .stacklet = raw_stacklet + offset,
        .stacklet_size = size - offset,
    };

    // TODO: vet constexpr usage in the library

    if !consteval {
      LF_ASSUME(is_sufficiently_aligned<k_new_align>(next.stacklet));
    }

    node_traits::construct(m_node_alloc, next_node, next);

    return next_node;
  }

  /**
   * @brief Delete a (possibly null) node and it's associated stacklet.
   */
  constexpr auto delete_node(node_ptr ptr) noexcept -> void {
    if (ptr != nullptr) {
      LF_ASSUME(ptr->original != nullptr);

      // Don't need to destroy (trivial destructor)
      byte_traits::deallocate(m_byte_alloc, ptr->raw_ptr, ptr->raw_size);

      node_traits::destroy(m_node_alloc, ptr);
      node_traits::deallocate(m_node_alloc, ptr, 1);
    }
  }
};

template <typename Allocator>
LF_NO_INLINE constexpr auto geometric<Allocator>::push_cached(std::size_t padded_size) -> void * {

  // Have to be very careful in this function to be strongly exception-safe!

  // This is the minimum size of node we could allocate that would fit the allocation.
  std::size_t min_node_size = padded_size + k_new_align - 1;

  if (m_root == nullptr) {
    // Fine if this throw
    heap_ptr new_root = heap_traits::allocate(m_heap_alloc, 1);

    // Can't throw
    heap_traits::construct(m_heap_alloc, new_root, 1);

    LF_TRY {
      new_root->top = new_node(nullptr, round_to_multiple<k_page_size>(min_node_size));
    } LF_CATCH_ALL {
      heap_traits::destroy(m_heap_alloc, new_root);
      heap_traits::deallocate(m_heap_alloc, new_root, 1);
      LF_RETHROW;
    }

    // Nothing can throw, safe to publish to *this.
    m_root = new_root;

    // Local copies of the new top.
    load_local();
    // Do the allocation.
    return std::exchange(m_sp, m_sp + padded_size);
  }

  LF_ASSUME(m_root->top != nullptr);

  if (m_root->cache != nullptr && m_root->cache->stacklet_size >= padded_size) {

    // We have space in the cache. No allocations on this path, nothing cam throw.

    if (m_sp == m_lo) {
      // There is nothing allocated on the current stacklet/top but it doesn't
      // have enough space hence, we need to delete top such that we don't end up
      // with an empty stacklet in the chain. This would break deletion otherwise.
      node_ptr empty_top = m_root->top;
      m_root->top = m_root->top->prev; // top could be null now
      delete_node(empty_top);
    }

    // Shuffle cache to the top.
    m_root->cache->prev = m_root->top;
    m_root->top = m_root->cache;
    m_root->cache = nullptr;

    // Local copies of the new top
    load_local();
    // Do the allocation.
    return std::exchange(m_sp, m_sp + padded_size);
  }

  // We need to allocate a new stacklet to fit this allocation, we choose to
  // grow geometrically to try to avoid too many allocations.
  std::size_t next_node_size = std::max(min_node_size, 2 * m_root->top->raw_size);

  // Fine if this throws
  node_ptr new_top = new_node(nullptr, round_to_multiple<k_page_size>(next_node_size));

  // Nothing can throw after this point

  // We didn't use the cache because it wasn't big enough, we should delete it
  // now because we had to grow the stack. We couldn't do this until now because
  // new_node may have thrown.
  delete_node(std::exchange(m_root->cache, nullptr));

  if (m_sp == m_lo) {
    // There is nothing allocated on the current stacklet/top but it doesn't
    // have enough space hence, we need to delete top such that we don't end up
    // with an empty stacklet in the chain. This would break deletion otherwise.
    node_ptr empty_top = m_root->top;
    m_root->top = m_root->top->prev; // top could be null now
    delete_node(empty_top);
  }

  // Commit the new/node
  new_top->prev = m_root->top;
  m_root->top = new_top;

  // Local copies of the new top
  load_local();
  // Do the allocation.
  return std::exchange(m_sp, m_sp + padded_size);
}

template <typename Allocator>
constexpr void geometric<Allocator>::pop_shuffle() noexcept {
  // Shuffle top/cache
  LF_ASSUME(m_root != nullptr);
  LF_ASSUME(m_root->top != nullptr);       // Pop from empty stack
  LF_ASSUME(m_root->top->prev != nullptr); // ^

  delete std::exchange(m_root->cache, m_root->top);
  m_root->top = m_root->top->prev;

  // Local copies of the new top
  m_lo = m_root->top->stacklet;
  m_sp = m_lo;
  m_hi = m_lo + m_root->top->size;
}

} // namespace lf::stack
