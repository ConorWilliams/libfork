#ifndef C39D8802_9977_423A_88EB_5816761ED5A8
#define C39D8802_9977_423A_88EB_5816761ED5A8

inline constexpr int primes_lim = 14;

/**
 * @brief See https://en.wikipedia.org/wiki/Primality_test
 */
inline constexpr auto is_prime = [](int n) -> bool {
  //
  if (n == 2 || n == 3) {
    return true;
  }

  if (n <= 1 || n % 2 == 0 || n % 3 == 0) {
    return false;
  }

  for (int i = 5; i * i <= n; i += 6) {
    if (n % i == 0 || n % (i + 2) == 0) {
      return false;
    }
  }

  return true;
};

#endif /* C39D8802_9977_423A_88EB_5816761ED5A8 */
