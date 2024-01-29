#ifndef C39D8802_9977_423A_88EB_5816761ED5A8
#define C39D8802_9977_423A_88EB_5816761ED5A8

#include <vector>

// Match serial

inline constexpr std::size_t fold_n /**/ = 10'000;
inline constexpr std::size_t fold_chunk = 1'000;
inline constexpr std::size_t fold_reps = 100'000;

inline auto make_vec_fold() -> std::vector<unsigned> {

  std::vector<unsigned> out(fold_n);

  unsigned count = 0;

  for (auto &&elem : out) {
    elem = ++count;
  }

  return out;
}

#endif /* C39D8802_9977_423A_88EB_5816761ED5A8 */
