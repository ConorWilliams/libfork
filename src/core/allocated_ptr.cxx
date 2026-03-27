export module libfork.core:allocated_ptr;

import std;

namespace lf {

template <typename T, typename Allocator = std::allocator<T>>
class allocated_ptr {
 public:
  using value_type = T;
  using allocator_type = Allocator;

 private:
  using allocator_traits = std::allocator_traits<Allocator>;
  using pointer = typename allocator_traits::pointer;

  [[no_unique_address]]
  Allocator m_alloc;
  pointer m_data = nullptr;
  std::size_t m_size = 0;

 public:
  constexpr auto size() const noexcept -> std::size_t { return m_size; }

  constexpr auto data() noexcept -> T * { return std::to_address(m_data); }
  constexpr auto data() const noexcept -> T const * { return std::to_address(m_data); }

 private:
  /**
   * @brief Destroy all element and deallocate memory
   */
  void clear_and_deallocate() noexcept {
    if (m_data) {
      if constexpr (not std::is_trivially_destructible_v<T>) {
        // If trivially destructible, we can skip destruction and just deallocate
        for (std::size_t i = 0; i < size_; ++i) {
          // std::to_address handles both raw pointers and "fancy" pointers
          allocator_traits::destroy(alloc_, std::to_address(ptr + i));
        }
      }
      allocator_traits::deallocate(alloc_, data_, size_);
    }
    m_data = nullptr;
    size_ = 0;
  }

 public:
  explicit allocated_ptr(Allocator const &alloc = Allocator()) : m_alloc(alloc), m_data(nullptr), size_(0) {}

  explicit allocated_ptr(std::size_t count, Allocator const &alloc = Allocator())
      : m_alloc(alloc),
        m_data(nullptr),
        size_(count) {
    if (count > 0) {
      data_ = allocator_traits::allocate(alloc_, size_);
      std::size_t i = 0;
      try {
        for (; i < size_; ++i) {
          allocator_traits::construct(alloc_, std::to_address(data_ + i));
        }
      } catch (...) {
        // Strong exception safety: Rollback if a constructor throws
        for (size_type j = i; j > 0; --j) {
          allocator_traits::destroy(alloc_, std::to_address(data_ + j - 1));
        }
        allocator_traits::deallocate(alloc_, data_, size_);
        throw;
      }
    }
  }

  // --- MOVE-ONLY ENFORCEMENT ---
  allocated_ptr(const allocated_ptr &) = delete;
  allocated_ptr &operator=(const allocated_ptr &) = delete;

  // Move constructor
  allocated_ptr(allocated_ptr &&other) noexcept
      : alloc_(std::move(other.alloc_)),
        data_(std::exchange(other.data_, nullptr)),
        size_(std::exchange(other.size_, 0)) {}

 private:
  static constexpr bool pocma = allocator_traits::propagate_on_container_move_assignment::value;

 public:
  allocated_ptr &
  operator=(allocated_ptr &&other) noexcept(pocma || allocator_traits::is_always_equal::value) {
    if (this == &other) {
      return *this;
    }

    if constexpr (pocma) {
      clear_and_deallocate();
      alloc_ = std::move(other.alloc_);
      data_ = std::exchange(other.data_, nullptr);
      size_ = std::exchange(other.size_, 0);
    } else {
      if (m_alloc == other.m_alloc) {
        clear_and_deallocate();
        data_ = std::exchange(other.data_, nullptr);
        size_ = std::exchange(other.size_, 0);
      } else {
        // POCMA is false and allocators are unequal: element-wise move assignment required
        pointer new_data = nullptr;
        if (other.size_ > 0) {
          new_data = allocator_traits::allocate(alloc_, other.size_);
          size_type i = 0;
          try {
            for (; i < other.size_; ++i) {
              allocator_traits::construct(alloc_, std::to_address(new_data + i), std::move(other.data_[i]));
            }
          } catch (...) {
            for (size_type j = i; j > 0; --j) {
              allocator_traits::destroy(alloc_, std::to_address(new_data + j - 1));
            }
            allocator_traits::deallocate(alloc_, new_data, other.size_);
            throw;
          }
        }

        clear_and_deallocate();
        m_data = new_data;
        size_ = other.size_;

        // Clear the source since we pilfered its contents,
        // leaving it in a valid but unspecified state.
        other.clear_and_deallocate();
      }
    }
    return *this;
  }

  // ------------------------------------------------------------------------
  // 4. Destructor
  // ------------------------------------------------------------------------
  ~allocated_ptr() { clear_and_deallocate(); }

  // ------------------------------------------------------------------------
  // 5. Container & AAC Methods
  // ------------------------------------------------------------------------

  allocator_type get_allocator() const noexcept { return m_alloc; }

  const_iterator cbegin() const noexcept { return m_data; }

  iterator end() noexcept { return data_ + size_; }
  const_iterator end() const noexcept { return data_ + size_; }
  const_iterator cend() const noexcept { return data_ + size_; }

  bool empty() const noexcept { return size_ == 0; }
  size_type size() const noexcept { return size_; }
  size_type max_size() const noexcept { return allocator_traits::max_size(alloc_); }

  reference operator[](size_type i) { return m_data[i]; }
  const_reference operator[](size_type i) const { return m_data[i]; }

  // ------------------------------------------------------------------------
  // 6. Swap
  // ------------------------------------------------------------------------
  void swap(allocated_ptr &other) noexcept(allocator_traits::propagate_on_container_swap::value ||
                                           allocator_traits::is_always_equal::value) {
    constexpr bool pocs = allocator_traits::propagate_on_container_swap::value;

    if constexpr (pocs) {
      using std::swap;
      swap(m_alloc, other.m_alloc);
    } else {
      // According to standard, swapping containers with unequal allocators
      // where POCS == false results in Undefined Behavior.
      assert(m_alloc == other.m_alloc && "Swap UB for unequal allocators without POCS");
    }
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
  }

  friend void swap(allocated_ptr &lhs, allocated_ptr &rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }
};

} // namespace lf
