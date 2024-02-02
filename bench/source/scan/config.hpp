#ifndef C39D8802_9977_423A_88EB_5816761ED5A8
#define C39D8802_9977_423A_88EB_5816761ED5A8

#include <vector>

// inline constexpr std::size_t scan_n /**/ = 1'000'000;
// inline constexpr std::size_t scan_chunk = 5'000;
// inline constexpr std::size_t scan_reps = 1'000;

// Big
// inline constexpr std::size_t scan_n /**/ = 100'000'000;
// inline constexpr std::size_t scan_chunk = scan_n / 33;
// inline constexpr std::size_t scan_reps = 10;

// Mid
inline constexpr std::size_t scan_n /**/ = 1'000'000;
inline constexpr std::size_t scan_chunk = scan_n / 32;
inline constexpr std::size_t scan_reps = 1'000;

// Really small
// inline constexpr std::size_t scan_n /**/ = 8'000;
// inline constexpr std::size_t scan_chunk = scan_n / 32;
// inline constexpr std::size_t scan_reps = 100'000;

inline auto make_vec() -> std::vector<unsigned> {

  std::vector<unsigned> out(scan_n);

  unsigned count = 0;

  for (auto &&elem : out) {
    elem = ++count;
  }

  return out;
}

#endif /* C39D8802_9977_423A_88EB_5816761ED5A8 */
