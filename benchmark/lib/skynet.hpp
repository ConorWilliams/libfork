#pragma once

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <cstdint>
#else
import std;
#endif

inline constexpr int skynet_branching = 10;

// Tree depth: total leaves = branching ** depth.
inline constexpr int skynet_test = 4; // 10^4 = 10'000 leaves
inline constexpr int skynet_base = 6; // 10^6 = 1'000'000 leaves

inline constexpr auto skynet_leaves(int depth) -> std::int64_t {
  std::int64_t out = 1;
  for (int i = 0; i < depth; ++i) {
    out *= skynet_branching;
  }
  return out;
}

inline constexpr auto skynet_expected(int depth) -> std::int64_t {
  std::int64_t leaves = skynet_leaves(depth);
  return leaves * (leaves - 1) / 2;
}
