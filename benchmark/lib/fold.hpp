#pragma once

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstddef>
  #include <vector>
#else
import std;
#endif

inline constexpr std::size_t fold_test = 1'000;
inline constexpr std::size_t fold_base = 1'000'000;

inline constexpr std::size_t fold_reps = 100;

inline auto fold_make_vec(std::size_t n) -> std::vector<unsigned> {
  std::vector<unsigned> out(n);
  unsigned count = 0;
  for (auto &elem : out) {
    elem = ++count;
  }
  return out;
}

// Sum of 1..n with n = vec.size().
inline constexpr auto fold_expected(std::size_t n) -> std::uint64_t {
  return static_cast<std::uint64_t>(n) * (n + 1) / 2;
}
