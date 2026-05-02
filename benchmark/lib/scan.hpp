#pragma once

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstddef>
  #include <vector>
#else
import std;
#endif

inline constexpr std::size_t scan_test = 1'000;
inline constexpr std::size_t scan_base = 8'000;

inline constexpr std::size_t scan_reps = 1'000;

inline auto scan_make_vec(std::size_t n) -> std::vector<unsigned> {
  std::vector<unsigned> out(n);
  unsigned count = 0;
  for (auto &elem : out) {
    elem = ++count;
  }
  return out;
}
