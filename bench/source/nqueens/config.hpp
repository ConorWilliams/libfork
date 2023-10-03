#ifndef FDBA8577_F021_4F7A_AB8B_91807F8F7688
#define FDBA8577_F021_4F7A_AB8B_91807F8F7688

/*
 * Check if queens positions conflict with each other.
 */
inline auto queens_ok(int n, char *a) -> bool {

  for (int i = 0; i < n; i++) {

    char p = a[i];

    for (int j = i + 1; j < n; j++) {
      if (char q = a[j]; q == p || q == p - (j - i) || q == p + (j - i)) {
        return false;
      }
    }
  }
  return true;
}

inline constexpr int nqueens_work = 13;

#endif /* FDBA8577_F021_4F7A_AB8B_91807F8F7688 */
