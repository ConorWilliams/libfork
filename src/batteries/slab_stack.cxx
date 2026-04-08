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
 * The ctrl metadata and usable stack space are fused into a single allocation: a header
 * at the front of the slab is followed immediately by the usable nodes.  There is no
 * segmentation, caching, or geometric growth — if the slab is full, push throws.
 *
 * For this to conform to `worker_stack` the allocators void pointer type must be `void *`
 */
export template <allocator_of<std::byte> Allocator = std::allocator<std::byte>>
class slab_stack {

  // Alignment unit — all allocations are a multiple of this size.
  struct alignas(k_new_align) node {};
  static_assert(sizeof(node) == k_new_align);

  using node_traits = std::allocator_traits<Allocator>::template rebind_traits<node>;
  using node_alloc_t = node_traits::allocator_type;
  using node_ptr = node_traits::pointer;
  using void_ptr = node_traits::void_pointer;
  using size_int = node_traits::size_type;
  using diff_int = node_traits::difference_type;

  // Fused ctrl+node header — lives at the very start of every slab allocation.
  // The usable stack space (size nodes) follows directly after the header region.
  struct slab {
    [[no_unique_address]]
    node_alloc_t node_alloc; // Propagated to new owners on acquire.
    node_ptr sp_cache;       // Stack pointer saved across release/acquire.
    diff_int size;           // Usable node count in this slab.
  };

  // Number of node-sized units occupied by the header at the front of each allocation.
  static constexpr diff_int k_header_nodes =
      safe_cast<diff_int>((sizeof(slab) + sizeof(node) - 1) / sizeof(node));

  // Default capacity: fill one page minus the header.
  static constexpr diff_int k_default_nodes =
      safe_cast<diff_int>(k_page_size / sizeof(node)) - k_header_nodes;

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
    explicit constexpr checkpoint_t(slab *ptr) noexcept : m_slab(ptr) {}
    slab *m_slab = nullptr;
  };

 public:
  constexpr slab_stack() : slab_stack(Allocator{}) {}
  explicit constexpr slab_stack(Allocator const &alloc, diff_int num_nodes = k_default_nodes)
      : m_alloc(alloc) {
    init_slab(num_nodes);
  }

  constexpr slab_stack(slab_stack const &) = delete;
  constexpr slab_stack(slab_stack &&) = delete;

  constexpr auto operator=(slab_stack const &) -> slab_stack & = delete;
  constexpr auto operator=(slab_stack &&) -> slab_stack & = delete;

  constexpr ~slab_stack() noexcept {
    LF_ASSUME(empty());
    free_slab(m_slab);
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
    return checkpoint_t{m_slab};
  }

  /**
   * @brief Allocate size bytes on the stack and return a pointer to the base of the allocation.
   */
  [[nodiscard]]
  constexpr auto push(std::size_t size) -> void_ptr {
    LF_ASSUME(size > 0);

    constexpr diff_int node_size = sizeof(node);

    diff_int push_bytes = safe_cast<diff_int>(round_to_multiple<sizeof(node)>(size));

    LF_ASSUME(push_bytes >= node_size);
    LF_ASSUME(push_bytes % node_size == 0);

    // Optimized to just the subtraction because multiplication cancels the implicit division.
    diff_int free_bytes = node_size * (m_hi - m_sp);

    if (push_bytes > free_bytes) [[unlikely]] {
      slab_full();
    }

    diff_int num_nodes = push_bytes / node_size;

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
  constexpr auto prepare_release() const noexcept -> release_t {
    // Guard against null release (failed prior allocation).
    if (m_slab != nullptr) {
      m_slab->sp_cache = m_sp;
    }
    return release_t{key()};
  }

  constexpr void release([[maybe_unused]] release_t) noexcept {
    diff_int next_size = (m_slab != nullptr) ? m_slab->size : k_default_nodes;

    // Hand off the current slab to whoever holds the checkpoint; clear local state.
    m_slab = nullptr;
    m_lo = nullptr;
    m_sp = nullptr;
    m_hi = nullptr;

    // Pre-allocate a fresh slab for our next use.  If this throws, swallow the
    // exception — push will see no space (m_hi - m_sp == 0) and throw instead.
    LF_TRY {
      init_slab(next_size);
    } LF_CATCH_ALL {
    }
  }

  constexpr void acquire(checkpoint_t ckpt) noexcept {
    LF_ASSUME(empty());

    if (ckpt.m_slab == nullptr) {
      return;
    }

    // Discard the fresh empty slab we prepared during release() (may be null on alloc failure).
    free_slab(m_slab);

    m_slab = ckpt.m_slab;

    if constexpr (!node_traits::is_always_equal::value) {
      m_alloc = node_alloc_t{std::as_const(m_slab->node_alloc)};
    }

    LF_ASSUME(m_slab != nullptr);

    load_local();
  }

 private:
  [[no_unique_address]]
  node_alloc_t m_alloc;

  slab *m_slab = nullptr;
  node_ptr m_lo = nullptr; // Base of usable space in the current slab.
  node_ptr m_sp = nullptr; // Stack pointer for the current slab.
  node_ptr m_hi = nullptr; // One-past-the-end of usable space in the current slab.

  // Restore local pointers from the slab header, taking sp from the cache.
  constexpr void load_local() noexcept {
    LF_ASSUME(m_slab != nullptr);
    node_ptr base = reinterpret_cast<node_ptr>(m_slab) + k_header_nodes;
    m_lo = base;
    m_hi = base + m_slab->size;
    m_sp = m_slab->sp_cache;
  }

  // Allocate and construct a fresh slab with num_nodes usable nodes.
  constexpr void init_slab(diff_int num_nodes) {
    LF_ASSUME(num_nodes > 0);

    size_int total = safe_cast<size_int>(k_header_nodes + num_nodes);
    node_ptr raw = node_traits::allocate(m_alloc, total);

    LF_TRY {
      m_slab = std::construct_at(reinterpret_cast<slab *>(std::to_address(raw)), m_alloc, nullptr, num_nodes);
    } LF_CATCH_ALL {
      node_traits::deallocate(m_alloc, raw, total);
      LF_RETHROW;
    }

    node_ptr base = raw + k_header_nodes;
    m_lo = m_sp = base;
    m_hi = base + num_nodes;
  }

  // Destroy and deallocate a slab (no-op if null).
  constexpr void free_slab(slab *s) noexcept {
    if (s != nullptr) {
      size_int total = safe_cast<size_int>(k_header_nodes + s->size);
      node_ptr raw = reinterpret_cast<node_ptr>(s);
      std::destroy_at(s);
      node_traits::deallocate(m_alloc, raw, total);
    }
  }

  [[noreturn]]
  static void slab_full() {
    LF_THROW(std::bad_alloc{});
  }
};

} // namespace lf
