#pragma once

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <array>
  #include <concepts>
  #include <cstddef>
  #include <cstdint>
  #include <iterator>
  #include <string_view>
  #include <type_traits>
#else
import std;
#endif

namespace lf_bench {

inline constexpr std::int64_t fold_test = 10;
inline constexpr std::int64_t fold_one_gib = 1024LL * 1024LL * 1024LL;

template <typename T>
inline constexpr std::int64_t fold_one_gib_elements = fold_one_gib / static_cast<std::int64_t>(sizeof(T));

template <typename T>
constexpr auto fold_base_sizes() {
  return std::array<std::int64_t, 9>{
      10,
      100,
      1'000,
      10'000,
      100'000,
      1'000'000,
      10'000'000,
      100'000'000,
      fold_one_gib_elements<T>,
  };
}

enum class fold_data_mode {
  memory,
  lazy,
};

enum class fold_chunk_mode {
  explicit_one,
  deduced,
  k1000,
};

enum class fold_projection_mode {
  sync,
  async,
};

template <typename T>
constexpr auto fold_dtype_name() -> std::string_view {
  if constexpr (std::same_as<T, std::int32_t>) {
    return "int32";
  } else if constexpr (std::same_as<T, float>) {
    return "float";
  } else {
    return "unknown";
  }
}

constexpr auto fold_data_name(fold_data_mode mode) -> std::string_view {
  switch (mode) {
    case fold_data_mode::memory:
      return "memory";
    case fold_data_mode::lazy:
      return "lazy";
  }
  return "unknown";
}

constexpr auto fold_chunk_name(fold_chunk_mode mode) -> std::string_view {
  switch (mode) {
    case fold_chunk_mode::explicit_one:
      return "chunk=1";
    case fold_chunk_mode::deduced:
      return "chunk=deduced";
    case fold_chunk_mode::k1000:
      return "chunk=1000";
  }
  return "unknown";
}

constexpr auto fold_projection_name(fold_projection_mode mode) -> std::string_view {
  switch (mode) {
    case fold_projection_mode::sync:
      return "sync_proj";
    case fold_projection_mode::async:
      return "async_proj";
  }
  return "unknown";
}

template <typename T>
struct ones_iterator {
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::random_access_iterator_tag;
  using iterator_concept = std::random_access_iterator_tag;

  difference_type pos = 0;

  constexpr auto operator*() const -> value_type { return value_type{1}; }
  constexpr auto operator[](difference_type) const -> value_type { return value_type{1}; }

  constexpr auto operator++() -> ones_iterator & {
    ++pos;
    return *this;
  }

  constexpr auto operator++(int) -> ones_iterator {
    auto ret = *this;
    ++*this;
    return ret;
  }

  constexpr auto operator--() -> ones_iterator & {
    --pos;
    return *this;
  }

  constexpr auto operator--(int) -> ones_iterator {
    auto ret = *this;
    --*this;
    return ret;
  }

  constexpr auto operator+=(difference_type n) -> ones_iterator & {
    pos += n;
    return *this;
  }

  constexpr auto operator-=(difference_type n) -> ones_iterator & {
    pos -= n;
    return *this;
  }

  friend constexpr auto operator+(ones_iterator it, difference_type n) -> ones_iterator {
    it += n;
    return it;
  }

  friend constexpr auto operator+(difference_type n, ones_iterator it) -> ones_iterator { return it + n; }

  friend constexpr auto operator-(ones_iterator it, difference_type n) -> ones_iterator {
    it -= n;
    return it;
  }

  friend constexpr auto operator-(ones_iterator lhs, ones_iterator rhs) -> difference_type {
    return lhs.pos - rhs.pos;
  }

  friend constexpr auto operator==(ones_iterator, ones_iterator) -> bool = default;
  friend constexpr auto operator<=>(ones_iterator, ones_iterator) = default;
};

template <typename T>
struct ones_range {
  using value_type = T;

  std::size_t count = 0;

  constexpr auto begin() const -> ones_iterator<T> { return {.pos = 0}; }

  constexpr auto end() const -> ones_iterator<T> {
    return {.pos = static_cast<typename ones_iterator<T>::difference_type>(count)};
  }

  constexpr auto size() const -> std::size_t { return count; }
};

template <typename T>
using fold_accum_t = std::conditional_t<std::same_as<T, float>, double, std::int64_t>;

} // namespace lf_bench
