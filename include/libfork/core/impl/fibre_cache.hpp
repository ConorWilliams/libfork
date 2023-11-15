#ifndef C4933A33_CEB1_4CD9_888C_888D34C3D990
#define C4933A33_CEB1_4CD9_888C_888D34C3D990

#include <algorithm>
#include <vector>

#include <libfork/core/macro.hpp>

#include "libfork/core/impl/fibre.hpp"

namespace lf::impl {

/**
 * @brief A container to cache empty fibres i.e. fibres with no allocations.
 */
class fibre_cache {

 public:
  /**
   * @brief Construct a new fibre cache capable of holding `size` fibres.
   */
  explicit fibre_cache(std::size_t cache_size) noexcept : m_cache_size(cache_size) {}

  /**
   * @brief Get the smallest available fibre.
   *
   * If no fibres are available, a new one is returned.
   */
  auto pop() -> fibre {
    if (m_heap.empty()) {
      return {};
    }

    std::pop_heap(m_heap.begin(), m_heap.end(), comp{});

    auto fibre = std::move(m_heap.back());
    m_heap.pop_back();
    return fibre;
  }

  /**
   * @brief Cache an empty fibre for later use if it is bigger than the smallest fibre.
   */
  void push(fibre &&fibre) {

    if (fibre.capacity() == 0 || m_cache_size == 0) {
      return;
    }

    fibre.squash();

    if (m_heap.size() < m_cache_size) {
      // If not full add to heap
      m_heap.push_back(std::move(fibre));
      std::push_heap(m_heap.begin(), m_heap.end(), comp{});

    } else if (fibre.capacity() > m_heap.front().capacity()) {
      // If bigger than the smallest fibre, replace the smallest fibre.
      std::pop_heap(m_heap.begin(), m_heap.end(), comp{});
      m_heap.back() = std::move(fibre);
      std::push_heap(m_heap.begin(), m_heap.end(), comp{});
    }
  }

 private:
  struct comp {
    LF_STATIC_CALL auto operator()(fibre const &lhs, fibre const &rhs) LF_STATIC_CONST noexcept -> bool {
      return lhs.capacity() > rhs.capacity();
    }
  };

  std::vector<fibre> m_heap;
  std::size_t m_cache_size;
};

} // namespace lf::impl

#endif /* C4933A33_CEB1_4CD9_888C_888D34C3D990 */
