#ifndef FDBA8577_F021_4F7A_AB8B_91807F8F7688
#define FDBA8577_F021_4F7A_AB8B_91807F8F7688

#include <array>

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

inline constexpr std::array<long, 28> answers = {
    0,
    1,
    0,
    0,
    2,
    10,
    4,
    40,
    92,
    352,
    724,
    2'680,
    14'200,
    73'712,
    365'596,
    2'279'184,
    14'772'512,
    95'815'104,
    666'090'624,
    4'968'057'848,
    39'029'188'884,
    314'666'222'712,
    2'691'008'701'644,
    24'233'937'684'440,
    227'514'171'973'736,
    2'207'893'435'808'352,
    22'317'699'616'364'044,
    234'907'967'154'122'528,
};

#endif /* FDBA8577_F021_4F7A_AB8B_91807F8F7688 */
