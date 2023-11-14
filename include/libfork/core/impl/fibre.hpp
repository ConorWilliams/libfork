#ifndef E43CF0B0_63CD_4E99_A7E1_96C331DB10C6
#define E43CF0B0_63CD_4E99_A7E1_96C331DB10C6

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>

#include "libfork/core/ext/ring_buffer.hpp"

#include "libfork/core/impl/utility.hpp"

namespace lf {

inline namespace ext {

class fibre {
 public:
  /**
   * @brief A fibril is a fibre fragment that contains a segment of the stack.
   *
   * A chain of fibrils looks like `R <- F1 <- F2 <- F3 <- ... <- Fn` where `R` is the root fibril.
   * Each fibril has a pointer to the root fibril and the root fibril has a pointer to the top
   * fibril, `Fn`.
   *
   */
  class fibril : impl::immovable<fibril> {

    struct deleter {
      void operator()(fibril *ptr) const noexcept {
        while (ptr != nullptr) {
          std::free(std::exchange(ptr, ptr->m_prev)); // NOLINT
        }
      }
    };

   public:
    /**
     * @brief A unique pointer to a fibril with a custom deleter.
     */
    using unique_ptr = std::unique_ptr<fibril, deleter>;

    /**
     * @brief Allocate a new fibril with a stack of size `size` and attach it to the given fibril chain.
     */
    [[nodiscard]] static auto next_fibril(std::size_t size, unique_ptr &&prev) -> unique_ptr {

      void *ptr = std::malloc(sizeof(fibril) + size); // NOLINT

      if (ptr == nullptr) {
        throw std::bad_alloc();
      }

      unique_ptr frag{static_cast<fibril *>(ptr)};

      frag->m_lo = impl::byte_cast(ptr) + sizeof(fibril);
      frag->m_hi = impl::byte_cast(ptr) + sizeof(fibril) + size;
      // SP is uninitialized.
      frag->m_link = prev ? prev->m_link : frag.get();
      frag->m_prev = prev.release();

      frag->root()->m_link = frag.get();

      return frag;
    }

   private:
    friend class fibre;

    std::byte *m_lo; ///< This fibril's stack.
    std::byte *m_sp; ///< The current position of the stack pointer in the stack.
    std::byte *m_hi; ///< The one-past-the-end address of the stack.

    fibril *m_link; ///< A non-owning pointer to root/top fibril if this is the other/root fibril.
    fibril *m_prev; ///< A linked list of fibrils.

    void *m_pad; ///< Padding to ensure that the stack is aligned.

    /**
     * @brief Get the first/root fibril in the chain.
     */
    [[nodiscard]] auto root() const noexcept -> fibril * { return impl::non_null(m_link); }

    /**
     * @brief Get the top fibril in the chain.
     */
    [[nodiscard]] auto top() const noexcept -> fibril * { return impl::non_null(root())->m_link; }
  };

  // Keep stack aligned.
  static_assert(sizeof(fibril) >= impl::k_new_align && sizeof(fibril) % impl::k_new_align == 0);
  // Implicit lifetime
  static_assert(std::is_trivially_default_constructible_v<fibril>);
  static_assert(std::is_trivially_destructible_v<fibril>);

 private:
  static constexpr std::size_t k_min_size = 128;

  // A null state occurs when m_control = nullptr, in this case m_lo == m_sp == m_hi == nullptr.

  fibril::unique_ptr m_top = {}; ///< The heap-allocated object that manages the fibre.

  std::byte *m_lo = nullptr; ///< Mirror of the top fibril's `m_lo` member.
  std::byte *m_sp = nullptr; ///< Position of stack pointer in the top fibril.
  std::byte *m_hi = nullptr; ///< Mirror of the top fibril's `m_sp` member.

  /**
   * @brief Size of the current fibre's stack.
   */
  [[nodiscard]] auto capacity() const noexcept -> std::size_t { return m_hi - m_lo; }

  /**
   * @brief Allocate a new fibril and attach it to the current fibre.
   *
   * Need to:
   *  - Maybe allocate a fibre_root-block.
   *  - Allocate a new fibril.
   *  - Attach the new fibril to the current fibre.
   *  - Copy fwd/bwd the SP and begin/end pointers.
   */
  void grow(std::size_t space) {

    // Round size to a power of 2 greater than `space`, min size and double the prev capacity.
    m_top = fibril::next_fibril(std::max({k_min_size, 2 * capacity(), space}), std::move(m_top));

    // Update mirrors.
    m_lo = m_top->m_lo;
    m_sp = m_top->m_lo; // m_top->m_sp is uninitialized.
    m_hi = m_top->m_hi;
  }

 public:
  /**
   * @brief Construct an empty fibre.
   */
  fibre() noexcept = default;

  /**
   * @brief Construct a new fibre object taking ownership of the fibre that `frag` is a part of.
   *
   * This requires that `frag` is part of a fibre chain that has called `release()`.
   */
  explicit fibre(fibril *frag)
      : m_top(frag->top()),
        m_lo(m_top->m_lo),
        m_sp(m_top->m_sp),
        m_hi(m_top->m_hi) {}

  fibre(fibre const &) = delete;

  auto operator=(fibre const &) -> fibre & = delete;

  fibre(fibre &&other) noexcept
      : m_top(std::move(other.m_top)),
        m_lo(std::exchange(other.m_lo, nullptr)),
        m_sp(std::exchange(other.m_sp, nullptr)),
        m_hi(std::exchange(other.m_hi, nullptr)) {}

  auto operator=(fibre &&other) noexcept -> fibre & {
    if (this != &other) {
      m_top = std::move(other.m_top);
      m_lo = std::exchange(other.m_lo, nullptr);
      m_sp = std::exchange(other.m_sp, nullptr);
      m_hi = std::exchange(other.m_hi, nullptr);
    }
    return *this;
  }

  ~fibre() noexcept = default;

  /**
   * @brief Release unused/unusable underlying storage.
   *
   * This does not reduce the capacity of the fibre. It requires that all allocations on
   * the current fibre have been deallocated.
   */
  auto squash() && -> fibre && {

    LF_ASSERT(m_sp == m_lo);

    if (m_top) {
      fibril::unique_ptr prev{m_top->m_prev}; // Takes ownership

      m_top->m_prev = nullptr;
      m_top->m_link = m_top.get();
    }

    return std::move(*this);
  }

  /**
   * @brief Release the underlying storage of the current fibre and make this fibre empty.
   *
   * Another bundle must call `attach` on a fibril of the released fibre to attach the fibre to
   * continue the released fibre.
   */
  [[nodiscard]] auto release() -> fibril * {
    if (m_top) {
      m_top->m_sp = m_sp;

      fibril *top = m_top.release();

      m_lo = nullptr;
      m_sp = nullptr;
      m_hi = nullptr;

      return top;
    }
    return nullptr;
  }

  /**
   * @brief Allocate `count` bytes of memory on a fibril in the bundle.
   *
   * The memory will be aligned to a multiple of `__STDCPP_DEFAULT_NEW_ALIGNMENT__`.
   *
   * Deallocate the memory with `deallocate` in a FILO manor.
   */
  [[nodiscard]] auto allocate(std::size_t size) -> void * {
    if (m_hi - m_sp < size) [[unlikely]] {
      grow(size);
    }

    LF_ASSERT(m_top);

    std::byte *prev = m_sp;
    m_sp += (size + impl::k_new_align - 1) & ~(impl::k_new_align - 1);
    return prev;
  }

  /**
   * @brief Deallocate `count` bytes of memory from the current fibre.
   *
   * This must be called in FILO order with `allocate`.
   */
  constexpr void deallocate(void *ptr) noexcept {
    // Should compile to a conditional move.
    m_sp = m_sp == m_lo ? m_sp : static_cast<std::byte *>(ptr);
  }

  /**
   * @brief Get the fibril that the last allocation was on.
   */
  [[nodiscard]] constexpr auto top() -> fibril * { return m_top.get(); }
};

} // namespace ext

} // namespace lf

namespace lf::impl {

/**
 * @brief A bundle manages a fibre which is composed of a chain of fibrils.
 */
class bundle : immovable<bundle> {

  static constexpr std::size_t k_init_size = 256;
};

} // namespace lf::impl

#endif /* E43CF0B0_63CD_4E99_A7E1_96C331DB10C6 */
