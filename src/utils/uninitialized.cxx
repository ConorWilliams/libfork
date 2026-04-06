
export module libfork.utils:uninitialized;

import std;

namespace lf {

// TODO: use trivial union

export template <typename T>
class uninitialized {
 public:
  constexpr uninitialized() = default;

  constexpr uninitialized(uninitialized const &) = delete;
  constexpr uninitialized(uninitialized &&) = delete;

  constexpr auto operator=(uninitialized const &) -> uninitialized & = delete;
  constexpr auto operator=(uninitialized &&) -> uninitialized & = delete;

  constexpr ~uninitialized() = default;

  auto operator->() noexcept -> T * { return std::launder(std::bit_cast<T *>(buffer)); }

  auto operator*() noexcept -> T & { return **this; }

  template <class... Args>
    requires std::constructible_from<T, Args...>
  auto construct(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> T & {
    return *::new (static_cast<void *>(buffer)) T(std::forward<Args>(args)...);
  }

  void destroy() noexcept { this->~T(); }

 private:
  alignas(std::max_align_t) std::byte buffer[sizeof(std::max_align_t)];
};

} // namespace lf
