#ifndef CCFF3C63_F96E_4159_92DE_84921182174B
#define CCFF3C63_F96E_4159_92DE_84921182174B

#include <iostream>

#include <libfork.hpp>

inline constexpr int work = 36;

inline constexpr auto sfib(int n) -> int {
  if (n < 2) {
    return n;
  }
  return sfib(n - 1) + sfib(n - 2);
};

#endif /* CCFF3C63_F96E_4159_92DE_84921182174B */
