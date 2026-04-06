module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.batteries:adaptor_stack;

import std;

import libfork.utils;

namespace lf {

/**
 * @brief An adaptor_stack wraps a standard allocator to satisfy the worker_stack concept.
 *
 * Every push/pop directly allocates/deallocates through the allocator. This is the simplest
 * possible stack implementation — no caching, no geometric growth — just a thin wrapper.
 *
 * For this to conform to `worker_stack` the allocators void pointer type must be `void *`.
 */
export template <allocator_of<std::byte> Allocator = std::allocator<std::byte>>
class adaptor_stack {

  struct ctrl;

  using ctrl_traits = std::allocator_traits<Allocator>::template rebind_traits<ctrl>;

  using ctrl_ptr = ctrl_traits::pointer;

  using void_ptr = std::allocator_traits<Allocator>::void_pointer;
  using size_int = std::allocator_traits<Allocator>::size_type;

  struct release_t {
    explicit constexpr release_t(key_t) noexcept {}
  };

  class checkpoint_t {
   public:
    constexpr checkpoint_t() noexcept = default;
    constexpr auto operator==(checkpoint_t const &) const noexcept -> bool = default;

   private:
    friend adaptor_stack;
    explicit constexpr checkpoint_t(ctrl_ptr ptr) noexcept : m_ctrl(ptr) {}
    ctrl_ptr m_ctrl = nullptr;
  };

 public:
  using allocator_type = Allocator;

  constexpr adaptor_stack() noexcept(noexcept(Allocator{})) : adaptor_stack(Allocator()) {}
  explicit constexpr adaptor_stack(Allocator const &alloc) noexcept : m_ctrl_alloc(alloc) {}

  constexpr adaptor_stack(adaptor_stack const &) = delete;
  constexpr adaptor_stack(adaptor_stack &&) = delete;

  constexpr auto operator=(adaptor_stack const &) -> adaptor_stack & = delete;
  constexpr auto operator=(adaptor_stack &&) -> adaptor_stack & = delete;

  constexpr ~adaptor_stack() noexcept {
    LF_ASSUME(empty());
    delete_ctrl(m_ctrl);
  }

  /**
   * @brief Test if the stack is empty (all pushes have been popped).
   */
  [[nodiscard]]
  constexpr auto empty() const noexcept -> bool {
    return m_depth == 0;
  }

  /**
   * @brief Get a checkpoint of the stack.
   */
  [[nodiscard]]
  constexpr auto checkpoint() noexcept -> checkpoint_t {
    return checkpoint_t{m_ctrl};
  }

  /**
   * @brief Allocate size bytes and return a pointer to the allocation.
   */
  [[nodiscard]]
  constexpr auto push(std::size_t size) -> void_ptr {
    LF_ASSUME(size > 0);

    size_int num_bytes = round_to_multiple<k_new_align>(size);

    if (m_ctrl == nullptr) {
      m_ctrl = new_ctrl();
    }

    LF_ASSUME(m_ctrl != nullptr);

    auto ptr = static_cast<void_ptr>(std::allocator_traits<Allocator>::allocate(m_ctrl->alloc, num_bytes));

    ++m_depth;

    return ptr;
  }

  /**
   * @brief Deallocate the allocation at ptr of size n.
   */
  constexpr void pop(void_ptr ptr, [[maybe_unused]] std::size_t n) noexcept {
    LF_ASSUME(!empty());
    LF_ASSUME(ptr != nullptr);
    LF_ASSUME(m_ctrl != nullptr);

    size_int num_bytes = round_to_multiple<k_new_align>(n);

    using alloc_ptr = std::allocator_traits<Allocator>::pointer;

    std::allocator_traits<Allocator>::deallocate(m_ctrl->alloc, static_cast<alloc_ptr>(ptr), num_bytes);

    --m_depth;
  }

  [[nodiscard]]
  constexpr auto prepare_release() const noexcept -> release_t {
    if (m_ctrl != nullptr) {
      m_ctrl->depth_cache = m_depth;
    }
    return release_t{key()};
  }

  constexpr void release([[maybe_unused]] release_t) noexcept {
    m_ctrl = nullptr;
    m_depth = 0;
  }

  constexpr void acquire(checkpoint_t ckpt) noexcept {
    LF_ASSUME(empty());
    LF_ASSUME(ckpt.m_ctrl != m_ctrl);

    if (ckpt.m_ctrl == nullptr) {
      return;
    }

    delete_ctrl(m_ctrl);

    m_ctrl = ckpt.m_ctrl;

    if constexpr (!ctrl_traits::is_always_equal::value) {
      m_ctrl_alloc = typename ctrl_traits::allocator_type{std::as_const(m_ctrl->alloc)};
    }

    m_depth = m_ctrl->depth_cache;
  }

 private:
  // ============== Types ==============  //

  struct ctrl {
    [[no_unique_address]]
    Allocator alloc;

    size_int depth_cache = 0;
  };

  // ============== Members ==============  //

  [[no_unique_address]]
  typename ctrl_traits::allocator_type m_ctrl_alloc;

  ctrl_ptr m_ctrl = nullptr;

  size_int m_depth = 0;

  // ============== Methods ==============  //

  [[nodiscard]]
  constexpr auto new_ctrl(this adaptor_stack &self) -> ctrl_ptr {
    ctrl_ptr ptr = ctrl_traits::allocate(self.m_ctrl_alloc, 1);

    LF_TRY {
      ctrl_traits::construct(self.m_ctrl_alloc, ptr, std::as_const(self.m_ctrl_alloc));
    } LF_CATCH_ALL {
      ctrl_traits::deallocate(self.m_ctrl_alloc, ptr, 1);
      LF_RETHROW;
    }

    return ptr;
  }

  constexpr void delete_ctrl(this adaptor_stack &self, ctrl_ptr ptr) noexcept {
    if (ptr != nullptr) {
      ctrl_traits::destroy(self.m_ctrl_alloc, ptr);
      ctrl_traits::deallocate(self.m_ctrl_alloc, ptr, 1);
    }
  }
};

} // namespace lf
