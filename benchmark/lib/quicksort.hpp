#pragma once

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstddef>
  #include <cstdint>
  #include <random>
  #include <vector>
#else
import std;
#endif

inline constexpr std::size_t quicksort_test = 10'000;
inline constexpr std::size_t quicksort_base = 10'000'000;

inline constexpr std::size_t quicksort_basecase = 32;

inline auto quicksort_make_input(std::size_t n, std::uint64_t seed = 0xDEADBEEF) -> std::vector<std::uint32_t> {
  std::vector<std::uint32_t> out(n);
  std::mt19937_64 rng{seed};
  std::uniform_int_distribution<std::uint32_t> dist;
  for (auto &v : out) {
    v = dist(rng);
  }
  return out;
}
