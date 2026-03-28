module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
#include <cstddef>
export module libfork.core:stack;

import std;

import :concepts;
import :constants;
import :utility;

namespace lf {

static auto make_bytes(std::size_t size) -> std::unique_ptr<std::byte[]> {
  return std::make_unique_for_overwrite<std::byte[]>(size);
}

export class geometric_stack {

  struct node;

  struct node_data : immovable {
    node *prev;                                               // Linked list (past).
    std::size_t size;                                         // Size of stacklet.
    std::unique_ptr<std::byte[]> stacklet = make_bytes(size); // Actual data.
    std::byte *sp_cache = nullptr;                            // Cached stack pointer for this stacklet.
  };

  // Align such that the entire node is on a cache line
  struct alignas(std::max(sizeof(node_data), alignof(node_data))) node : node_data {
    constexpr node(node *prev, std::size_t size) : node_data{.prev = prev, .size = size} {}
  };

  static_assert(sizeof(node) == alignof(node));
  static_assert(alignof(node) <= k_cache_line);

  struct heap : immovable {

    std::stack<std::byte *> debug; // Debugging FILO tracking.
    node *top;                     // Most recent stacklet i.e. the top of the stack, never null.
    node *cache = nullptr;         // Cached (empty) stacklet for hot-split gaurding.

    explicit constexpr heap(std::size_t size) : top(new node{nullptr, size}) {}
  };

  class checkpoint_t {
   public:
    auto operator==(checkpoint_t const &) const noexcept -> bool = default;

   private:
    explicit constexpr checkpoint_t(heap *root) noexcept : m_root(root) {}
    friend class geometric_stack;
    heap *m_root;
  };

 public:
  // export template <typename T>
  // * - After construction `this` is in the empty state and push is valid.
  // * - Pop is valid provided the FILO order is respected.
  // * - Push produces pointers aligned to __STDCPP_DEFAULT_NEW_ALIGNMENT__.
  // * - Destruction is expected to only occur when the stack is empty.
  // * - Result of `.checkpoint()` is expected to:
  // *     - Be "cheap to copy".
  // *     - Compare equal if they belong to the same stack.
  // * - Release detaches the current stack and leaves `this` in the empty state.
  // * - Acquire attaches to the stack that the checkpoint came from:
  // *     - This is a noop if the checkpoint is from the current stack.
  // *     - Otherwise `this` is empty.
  // *
  //  concept stack_allocator = std::is_object_v<T> && requires (T allocator, std::size_t n, void *ptr) {
  //   // { alloc.empty() } noexcept -> std::same_as<bool>;
  //   { allocator.push(n) } -> std::same_as<void *>;
  //   { allocator.pop(ptr, n) } noexcept -> std::same_as<void>;
  // };

  [[nodiscard]]
  constexpr auto checkpoint() noexcept -> checkpoint_t {
    return checkpoint_t{m_root.get()};
  }

  constexpr void release() noexcept {
    std::ignore = m_root.release();
    m_sp = nullptr;
    m_hi = nullptr;
  }

  constexpr void acquire(checkpoint_t ckpt) noexcept {
    if (ckpt.m_root != m_root.get() && ckpt.m_root != nullptr) {
      LF_ASSUME(m_root == nullptr); // Should only acquire when empty.
      LF_ASSUME(m_root->top);       // Should never be nullptr
      m_root.reset(ckpt.m_root);
      m_sp = m_root->top->sp_cache;
      m_hi = m_root->top->stacklet.get() + m_root->top->size;
    }
  }

  std::unique_ptr<heap> m_root = nullptr;
  std::byte *m_sp = nullptr;
  std::byte *m_hi = nullptr;
};

/**
 * @brief Round size close to a multiple of the page_size.
 */
[[nodiscard]]
constexpr auto round_up_to_page_size(std::size_t size) noexcept -> std::size_t {

  // Want calculate req such that:

  // req + malloc_block_est is a multiple of the page size.
  // req > size + stacklet_size

  std::size_t constexpr page_size = 4096;                           // 4 KiB on most systems.
  std::size_t constexpr malloc_meta_data_size = 6 * sizeof(void *); // An (over)estimate.

  std::size_t minimum = size + malloc_meta_data_size;
  std::size_t rounded = (minimum + page_size - 1) & ~(page_size - 1);
  std::size_t request = rounded - malloc_meta_data_size;

  LF_ASSUME(minimum <= rounded);
  LF_ASSUME(rounded % page_size == 0);
  LF_ASSUME(request >= size);

  return request;
}

/**
 * @brief A stack is a user-space (geometric) segmented program stack.
 *
 * A stack stores the execution of a DAG from root (which may be a stolen task or true root) to suspend
 * point. A stack is composed of stacklets, each stacklet is a contiguous region of stack space stored in a
 * double-linked list. A stack tracks the top stacklet, the top stacklet contains the last allocation or the
 * stack is empty. The top stacklet may have zero or one cached stacklets "ahead" of it.
 */
export class stack : immovable {

 public:
  /**
   * @brief A stacklet is a stack fragment that contains a segment of the stack.
   *
   * A chain of stacklets looks like `R <- F1 <- F2 <- F3 <- ... <- Fn` where `R` is the root stacklet.
   *
   * A stacklet is allocated as a contiguous chunk of memory, the first bytes of the chunk contain the
   * stacklet object. Semantically, a stacklet is a dynamically sized object.
   */
  class alignas(k_new_align) stacklet : immovable {

    friend class stack;

    /**
     * @brief Capacity of the current stacklet's stack.
     */
    [[nodiscard]]
    auto capacity() const noexcept -> std::size_t {
      LF_ASSUME(m_hi - m_lo >= 0);
      return static_cast<std::size_t>(m_hi - m_lo);
    }
    /**
     * @brief Unused space on the current stacklet's stack.
     */
    [[nodiscard]]
    auto unused() const noexcept -> std::size_t {
      LF_ASSUME(m_hi - m_sp >= 0);
      return static_cast<std::size_t>(m_hi - m_sp);
    }
    /**
     * @brief Check if stacklet's stack is empty.
     */
    [[nodiscard]]
    auto empty() const noexcept -> bool {
      return m_sp == m_lo;
    }
    /**
     * @brief Check is this stacklet is the top of a stack.
     */
    [[nodiscard]]
    auto is_top() const noexcept -> bool {
      if (m_next != nullptr) {
        // Accept a single cached stacklet above the top.
        return m_next->empty() && m_next->m_next == nullptr;
      }
      return true;
    }
    /**
     * @brief Set the next stacklet in the chain to 'new_next'.
     *
     * This requires that this is the top stacklet. If there is a cached stacklet ahead of the top stacklet
     * then it will be freed before being replaced with 'new_next'.
     */
    void set_next(stacklet *new_next) noexcept {
      LF_ASSUME(is_top());
      std::free(std::exchange(m_next, new_next));
    }
    /**
     * @brief Allocate a new stacklet with a stack of size of at least`size` and attach it to the given
     * stacklet chain.
     *
     * Requires that `prev` must be the top stacklet in a chain or `nullptr`.
     */
    [[nodiscard]]
    LF_NO_INLINE static auto next_stacklet(std::size_t size, stacklet *prev) -> stacklet * {

      LF_ASSUME(prev == nullptr || prev->is_top());

      std::size_t request = round_up_to_page_size(size + sizeof(stacklet));

      LF_ASSUME(request >= sizeof(stacklet) + size);

      stacklet *next = static_cast<stacklet *>(std::malloc(request));

      if (next == nullptr) {
        throw std::bad_alloc();
      }

      if (prev != nullptr) {
        // Set next tidies up other next.
        prev->set_next(next);
      }

      next->m_lo = std::bit_cast<std::byte *>(next) + sizeof(stacklet);
      next->m_sp = next->m_lo;
      next->m_hi = std::bit_cast<std::byte *>(next) + request;

      next->m_prev = prev;
      next->m_next = nullptr;

      return next;
    }

    /**
     * @brief Allocate an initial stacklet.
     */
    [[nodiscard]]
    static auto next_stacklet() -> stacklet * {
      return stacklet::next_stacklet(1, nullptr);
    }

    /**
     * @brief This stacklet's stack.
     */
    std::byte *m_lo;
    /**
     * @brief The current position of the stack pointer in the stack.
     */
    std::byte *m_sp;
    /**
     * @brief The one-past-the-end address of the stack.
     */
    std::byte *m_hi;
    /**
     * @brief Doubly linked list (past).
     */
    stacklet *m_prev;
    /**
     * @brief Doubly linked list (future).
     */
    stacklet *m_next;
  };

  /**
   * @brief Constructs a stack with a small empty stack.
   */
  stack() : m_fib(stacklet::next_stacklet()) {}

  /**
   * @brief Destroy the stack object.
   */
  ~stack() noexcept {
    LF_ASSUME(m_fib);
    LF_ASSUME(!m_fib->m_prev); // Should only be destructed at the root.
    m_fib->set_next(nullptr);  // Free a cached stacklet.
    std::free(m_fib);
  }

  /**
   * @brief Allocate `size` bytes of memory on a stacklet.
   *
   * The memory will be aligned to a multiple of `k_new_align`.
   */
  [[nodiscard]]
  LF_FORCE_INLINE inline auto push(std::size_t size) -> void * {
    LF_ASSUME(m_fib && m_fib->is_top());

    // Round up to the next multiple of the alignment.
    std::size_t ext_size = (size + k_new_align - 1) & ~(k_new_align - 1);

    if (m_fib->unused() < ext_size) {
      if (m_fib->m_next != nullptr && m_fib->m_next->capacity() >= ext_size) {
        m_fib = m_fib->m_next;
      } else {
        m_fib = stacklet::next_stacklet(std::max(2 * m_fib->capacity(), ext_size), m_fib);
      }
    }

    LF_ASSUME(m_fib && m_fib->is_top());

    return std::exchange(m_fib->m_sp, m_fib->m_sp + ext_size);
  }

  /**
   * @brief Deallocate memory from the current stack.
   *
   * This must be called in FILO order with `push`.
   */
  LF_FORCE_INLINE inline void pop(void *ptr, [[maybe_unused]] std::size_t n) noexcept {

    LF_ASSUME(m_fib && m_fib->is_top());

    m_fib->m_sp = static_cast<std::byte *>(ptr);

    if (m_fib->empty()) {

      if (m_fib->m_prev != nullptr) {
        // Always free a second order cached stacklet if it exists.
        m_fib->set_next(nullptr);
        // Move to prev stacklet.
        m_fib = m_fib->m_prev;
      }

      LF_ASSUME(m_fib);

      // Guard against over-caching.
      if (m_fib->m_next != nullptr) {
        if (m_fib->m_next->capacity() > 8 * m_fib->capacity()) {
          // Free oversized stacklet.
          m_fib->set_next(nullptr);
        }
      }
    }

    LF_ASSUME(m_fib && m_fib->is_top());
  }

  /**
   * @brief Get the checkpoint of the current stack.
   */
  [[nodiscard]]
  auto checkpoint() const noexcept -> stacklet * {
    LF_ASSUME(m_fib && m_fib->is_top());
    return m_fib;
  }

  /**
   * @brief Detach the current stack and leave this in the empty state.
   */
  void release() noexcept {
    LF_ASSUME(m_fib && m_fib->is_top());
    m_fib = stacklet::next_stacklet();
  }

  /**
   * @brief Attach to the stack that the checkpoint came from.
   */
  void acquire(stacklet *frag) noexcept {
    if (m_fib == frag) {
      return;
    }
    LF_ASSUME(m_fib->empty() && m_fib->m_prev == nullptr);
    m_fib->set_next(nullptr);
    std::free(m_fib);
    m_fib = frag;
  }

 private:
  /**
   * @brief The allocation stacklet.
   */
  stacklet *m_fib;
};

static_assert(stack_allocator<stack>);

} // namespace lf
