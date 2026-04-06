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

  struct alignas(k_new_align) aligned {};

  using align_trait = std::allocator_traits<Allocator>::template rebind_traits<aligned>;
  using align_alloc = align_trait::allocator_type;
  using alloc_ptr = align_trait::pointer;

  using void_ptr = align_trait::void_pointer;
  using size_int = align_trait::size_type;

  struct release_t {
    explicit constexpr release_t(key_t) noexcept {}
  };

  class checkpoint_t {
   public:
    constexpr checkpoint_t() noexcept = default;
    constexpr auto operator==(checkpoint_t const &) const noexcept -> bool = default;

   private:
    struct empty {
      constexpr empty() noexcept = default;
      constexpr auto operator==(empty const &) const noexcept -> bool = default;
      explicit constexpr empty(align_alloc const &) noexcept {};
    };

    friend adaptor_stack;
    explicit constexpr checkpoint_t(align_alloc const &alloc) noexcept : m_alloc(alloc) {}

    std::conditional_t<align_trait::is_always_equal::value, empty, align_alloc> m_alloc;
  };

 public:
  using allocator_type = Allocator;

  constexpr adaptor_stack() noexcept(noexcept(Allocator{})) : adaptor_stack(Allocator()) {}
  explicit constexpr adaptor_stack(Allocator const &alloc) noexcept : m_alloc(alloc) {}

  // TODO: drop constexpr for =default and =delete across the lib

  constexpr adaptor_stack(adaptor_stack const &) = delete;
  constexpr adaptor_stack(adaptor_stack &&) = delete;

  constexpr auto operator=(adaptor_stack const &) -> adaptor_stack & = delete;
  constexpr auto operator=(adaptor_stack &&) -> adaptor_stack & = delete;

  /**
   * @brief Get a checkpoint of the stack.
   */
  [[nodiscard]]
  constexpr auto checkpoint() noexcept -> checkpoint_t {
    return checkpoint_t{m_alloc};
  }

  /**
   * @brief Allocate size bytes and return a pointer to the allocation.
   */
  [[nodiscard]]
  constexpr auto push(std::size_t size) -> void_ptr {
    LF_ASSUME(size > 0);
    size_int num_aligned = (size + (k_new_align - 1)) / sizeof(aligned);
    return static_cast<void_ptr>(std::allocator_traits<Allocator>::allocate(m_alloc, num_aligned));
  }

  /**
   * @brief Deallocate the allocation at ptr of size n.
   */
  constexpr void pop(void_ptr ptr, [[maybe_unused]] std::size_t size) noexcept {
    LF_ASSUME(size > 0);
    size_int num_aligned = (size + (k_new_align - 1)) / sizeof(aligned);
    std::allocator_traits<Allocator>::deallocate(m_alloc, static_cast<alloc_ptr>(ptr), num_aligned);
  }

  [[nodiscard]]
  constexpr auto prepare_release() const noexcept -> release_t {
    return release_t{key()};
  }

  constexpr void release([[maybe_unused]] release_t) noexcept {}

  constexpr void acquire(checkpoint_t ckpt) noexcept {}

 private:
  Allocator m_alloc;
};

} // namespace lf
