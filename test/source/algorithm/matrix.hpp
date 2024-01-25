#ifndef FC8523B7_91E1_4380_ADE2_512163CE73AF
#define FC8523B7_91E1_4380_ADE2_512163CE73AF

#include <compare>
#include <ostream>
#include <random>
#include <vector>

#include "libfork/schedule/ext/random.hpp"

// Non commuting class

struct matrix {
  unsigned long a, b, c, d;

  auto operator<=>(matrix const &) const = default;

  friend auto operator<<(std::ostream &os, matrix const &mat) -> std::ostream & {
    return os << '{' << mat.a << ',' << mat.b << ',' << mat.c << ',' << mat.d << '}';
  }
};

auto operator*(matrix const &lhs, matrix const &rhs) -> matrix {
  return {
      lhs.a * rhs.a + lhs.b * rhs.c,
      lhs.a * rhs.b + lhs.b * rhs.d,
      lhs.c * rhs.a + lhs.d * rhs.c,
      lhs.c * rhs.b + lhs.d * rhs.d,
  };
}

auto operator*(int a, matrix const &rhs) -> matrix {
  return {
      a * rhs.a,
      a * rhs.b,
      a * rhs.c,
      a * rhs.d,
  };
}
auto random_vec(std::type_identity<matrix>, std::size_t n) -> std::vector<matrix> {

  std::vector<matrix> out(n);

  lf::xoshiro rng{lf::seed, std::random_device{}};
  std::uniform_int_distribution<unsigned long> dist{0, 100};

  for (auto &&elem : out) {
    elem = matrix{
        dist(rng),
        dist(rng),
        dist(rng),
        dist(rng),
    };
  }

  return out;
}

#endif /* FC8523B7_91E1_4380_ADE2_512163CE73AF */
