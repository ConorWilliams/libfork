#ifndef C39D8802_9977_423A_88EB_5816761ED5A8
#define C39D8802_9977_423A_88EB_5816761ED5A8

#include <vector>

inline constexpr std::size_t scan_n /**/ = 10'000;
inline constexpr std::size_t scan_chunk = 1'000;
inline constexpr std::size_t scan_reps = 10'000;

inline auto make_vec() -> std::vector<unsigned> {

  std::vector<unsigned> out(scan_n);

  unsigned count = 0;

  for (auto &&elem : out) {
    elem = ++count;
  }

  return out;
}

#endif /* C39D8802_9977_423A_88EB_5816761ED5A8 */
