#pragma once

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <array>
  #include <cstdint>
#else
import std;
#endif

inline constexpr int nqueens_test = 8;
inline constexpr int nqueens_base = 14;

inline constexpr std::array<std::int64_t, 28> nqueens_answers = {
    0,    1,         0,          0,           2,            10,            4,           40,
    92,   352,       724,        2'680,       14'200,       73'712,        365'596,     2'279'184,
    14'772'512,      95'815'104,  666'090'624, 4'968'057'848, 39'029'188'884,
};

inline auto queens_ok(int n, char const *a) -> bool {
  for (int i = 0; i < n; ++i) {
    char p = a[i];
    for (int j = i + 1; j < n; ++j) {
      char q = a[j];
      if (q == p || q == p - (j - i) || q == p + (j - i)) {
        return false;
      }
    }
  }
  return true;
}
