#include <format>
#include <stdexcept>

struct incorrect_result : public std::runtime_error {
  template <class... Args>
  explicit constexpr incorrect_result(std::format_string<Args...> fmt, Args &&...args)
      : std::runtime_error(std::format(fmt, std::forward<Args>(args)...)) {}
};
