
export module libfork.utils:uninitialized;

import std;

import :concepts;

namespace lf {

#if defined(__cpp_trivial_union) && __cpp_trivial_union >= 202306L
  #pragma message("TODO: __cpp_trivial_union is available — remove union workaround")
#endif

export template <plain_object T>
class uninitialized {
 public:
  constexpr uninitialized() = default;

  constexpr uninitialized(uninitialized const &) = delete;
  constexpr uninitialized(uninitialized &&) = delete;

  constexpr auto operator=(uninitialized const &) -> uninitialized & = delete;
  constexpr auto operator=(uninitialized &&) -> uninitialized & = delete;

  constexpr ~uninitialized() = default;

  auto operator->() noexcept -> T * { return std::launder(std::bit_cast<T *>(auto{buffer})); }

  auto operator*() noexcept -> T & { return *this->operator->(); }

  template <class... Args>
    requires std::constructible_from<T, Args...>
  auto construct(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> T & {
    return *::new (static_cast<void *>(buffer)) T(std::forward<Args>(args)...);
  }

  void destroy() noexcept { (**this).~T(); }

 private:
  alignas(T) std::byte buffer[sizeof(T)];
};

} // namespace lf
