
#include <algorithm>                             // for min
#include <catch2/catch_template_test_macros.hpp> // for TEMPLATE_TEST_CASE, TypeList
#include <catch2/catch_test_macros.hpp>          // for operator<=, operator==, INTERNAL_CATCH_...
#include <concepts>                              // for constructible_from
#include <cstddef>                               // for size_t
#include <functional>                            // for plus, identity, multiplies
#include <iostream>
#include <limits>  // for numeric_limits
#include <numeric> // for inclusive_scan
#include <random>  // for random_device, uniform_int_distribution
#include <stdexcept>
#include <string>      // for operator+, string, basic_string
#include <thread>      // for thread
#include <type_traits> // for type_identity
#include <utility>     // for forward
#include <vector>      // for operator==, vector

#include "libfork/core.hpp"

namespace {

enum class side {
  lhs, ///<
  rhs  ///<
};

inline constexpr std::size_t Fan = 2;

// /**
//  * Requires dividing as few as `(n + 1)` elements into `Fan` sections, hence, `n + 1 >= Fan`.
//  */
// template <std::random_access_iterator I,
//           std::sized_sentinel_for<I> S,
//           std::random_access_iterator O,
//           class Proj,
//           class Bop>
// auto reduction_sweep(I beg, S end, std::iter_difference_t<I> n, Bop bop, Proj proj, O out) {
//   //
//   std::iter_difference_t<I> const size = end - beg;

//   if (size <= n) {

//     std::iter_value_t<O> acc = proj(*beg);
//     *out = acc;

//     for (++beg, ++out; beg != end; ++beg, ++out) {
//       *out = acc = bop(std::move(acc), proj(*beg));
//     }

//     return;
//   }

//   // Recurse into Fan chunks.

//   static_assert(Fan >= 2, "Recursion must reduce size");

//   if (n + 1 < Fan) {
//     throw std::runtime_error("Fan too large for this chunk size!");
//   }

//   std::iter_difference_t<I> const segs = Fan;
//   std::iter_difference_t<I> const chunk = size / segs;
//   std::iter_difference_t<I> const last = chunk * (Fan - 1);

//   { // Recursion

//     // #pragma unroll(Fan) experiment
//     for (std::iter_difference_t<I> i = 0; i < Fan - 1; ++i) {

//       auto a = chunk * i;
//       auto b = chunk * (i + 1);

//       reduction_sweep(beg + a, beg + b, n, bop, proj, out + a);
//     }
//     reduction_sweep(beg + last, end, n, bop, proj, out + last);
//   }

//   { // Scan over the fan results and write to out.

//     std::iter_value_t<O> acc = *(out + chunk - 1);

//     for (std::iter_difference_t<I> i = 1; i < Fan - 1; ++i) {

//       auto a = chunk * i;
//       auto b = chunk * (i + 1);

//       *(out + b - 1) = acc = bop(std::move(acc), *(out + b - 1)); // MOVE here
//     }
//     *(out + size - 1) = bop(std::move(acc), *(out + size - 1)); // MOVE here
//   }
// }

// template <side Side = side::lhs,         // Always start with an lhs scan-sweep.
//           std::random_access_iterator O, //
//           std::sized_sentinel_for<O> S,
//           class Bop>
// auto scan_sweep(O beg, S end, std::iter_difference_t<O> n, Bop bop) -> void {

//   std::iter_difference_t<O> const size = end - beg;

//   // LF_ASSERT(size >= 1);

//   if (size <= n) {
//     if constexpr (Side == side::rhs) {

//       std::iter_value_t<O> carry = *(beg - 1);

//       for (auto last = beg + size - 1; beg != last; ++beg) {
//         *beg = bop(carry, *beg);
//       }
//     }
//     return;
//   }

//   // Recurse into Fan chunks.

//   static_assert(Fan >= 2, "Recursion must reduce size");

//   if (n + 1 < Fan) {
//     throw std::runtime_error("Fan too large for this chunk size!");
//   }

//   std::iter_difference_t<O> const segs = Fan;
//   std::iter_difference_t<O> const chunk = size / segs;
//   std::iter_difference_t<O> const last = chunk * (Fan - 1);

//   if constexpr (Side == side::rhs) {

//     // Rhs needs to carry left siblings.

//     std::iter_value_t<O> acc = *(beg - 1);

//     // Effectivly a strided inclusive scan.
//     for (std::iter_difference_t<O> i = 0; i < Fan - 1; ++i) {

//       auto a = chunk * i;
//       auto b = chunk * (i + 1);

//       // Carry left sibling (addative)
//       *(beg + b - 1) = bop(std::move(acc), *(beg + b - 1));
//     }
//   }

//   if constexpr (Side == side::lhs) {
//     // LHS's propagets to first child.
//     scan_sweep<side::lhs>(beg, beg + chunk, n, bop);
//   }

//   constexpr std::iter_difference_t<O> skip = Side == side::lhs ? 1 : 0;

//   for (std::iter_difference_t<O> i = skip; i < Fan - 1; ++i) {

//     auto a = chunk * i;
//     auto b = chunk * (i + 1);

//     scan_sweep<side::rhs>(beg + a, beg + b, n, bop);
//   }
//   scan_sweep<side::rhs>(beg + last, end, n, bop);
// }

/**
 * Requires dividing as few as `(n + 1)` elements into `Fan` sections, hence, `n + 1 >= Fan`.
 */
template <side Side = side::rhs,
          std::random_access_iterator I,
          std::sized_sentinel_for<I> S,
          std::random_access_iterator O,
          class Proj,
          class Bop>
auto reduction_sweep2(I beg, S end, std::iter_difference_t<I> n, Bop bop, Proj proj, O out) ->

    void

{
  //
  std::iter_difference_t<I> const size = end - beg;

  if (size <= n) {

    if constexpr (Side == side::rhs) {
      return;
    }

    std::iter_value_t<O> acc = proj(*beg);

    for (++beg; beg != end; ++beg) {
      acc = bop(std::move(acc), proj(*beg));
    }

    *(out + size - 1) = std::move(acc);

    return;
  }

  // Recurse into Fan chunks.

  static_assert(Fan >= 2, "Recursion must reduce size");

  if (n + 1 < Fan) {
    throw std::runtime_error("Fan too large for this chunk size!");
  }

  std::iter_difference_t<I> const segs = Fan;
  std::iter_difference_t<I> const chunk = size / segs;
  std::iter_difference_t<I> const last = chunk * (Fan - 1);

  { // Recursion into fan segements.

    // #pragma unroll(Fan) experiment
    for (std::iter_difference_t<I> i = 0; i < Fan - 1; ++i) {

      auto a = chunk * i;
      auto b = chunk * (i + 1);

      reduction_sweep2<side::lhs>(beg + a, beg + b, n, bop, proj, out + a);
    }
    reduction_sweep2<Side>(beg + last, end, n, bop, proj, out + last);
  }

  { // Scan over the fan results and write to out.

    std::iter_value_t<O> acc = *(out + chunk - 1);

    for (std::iter_difference_t<I> i = 1; i < Fan - 1; ++i) {

      auto a = chunk * i;
      auto b = chunk * (i + 1);

      *(out + b - 1) = acc = bop(std::move(acc), *(out + b - 1)); // MOVE here
    }
    if constexpr (Side == side::lhs) {
      *(out + size - 1) = bop(std::move(acc), *(out + size - 1)); // MOVE here
    }
  }
}

template <side Side = side::lhs, // Always start with an lhs scan-sweep.
          std::random_access_iterator I,
          std::sized_sentinel_for<I> S,
          std::random_access_iterator O, //
          class Proj,
          class Bop>
auto scan_sweep(I beg, S end, std::iter_difference_t<I> n, Bop bop, Proj proj, O out) -> void {

  std::iter_difference_t<I> const size = end - beg;

  if (size <= n) {

    auto last = beg + size - 1;

    if constexpr (Side == side::rhs) {
      std::transform_inclusive_scan(beg, last, out, bop, proj, *(out - 1));
    } else {
      std::transform_inclusive_scan(beg, last, out, bop, proj);
    }
    return;
  }

  // Recurse into Fan chunks.

  static_assert(Fan >= 2, "Recursion must reduce size");

  if (n + 1 < Fan) {
    throw std::runtime_error("Fan too large for this chunk size!");
  }

  std::iter_difference_t<O> const segs = Fan;
  std::iter_difference_t<O> const chunk = size / segs;
  std::iter_difference_t<O> const last = chunk * (Fan - 1);

  if constexpr (Side == side::rhs) {

    // Rhs needs to carry left siblings.

    std::iter_value_t<O> acc = *(out - 1);

    for (std::iter_difference_t<O> i = 0; i < Fan - 1; ++i) {

      auto a = chunk * i;
      auto b = chunk * (i + 1);

      // Carry left sibling (addative)
      *(out + b - 1) = bop(std::move(acc), *(out + b - 1));
    }
  }

  if constexpr (Side == side::lhs) {
    // LHS's propagets to first child.
    scan_sweep<side::lhs>(beg, beg + chunk, n, bop, proj, out);
  }

  constexpr std::iter_difference_t<O> skip = Side == side::lhs ? 1 : 0;

  for (std::iter_difference_t<O> i = skip; i < Fan - 1; ++i) {

    auto a = chunk * i;
    auto b = chunk * (i + 1);

    scan_sweep<side::rhs>(beg + a, beg + b, n, bop, proj, out + a);
  }
  scan_sweep<side::rhs>(beg + last, end, n, bop, proj, out + last);
}

} // namespace

TEMPLATE_TEST_CASE("play", "[play][template]", (std::integral_constant<int, 1>)) {

  std::array arr = {1, 1, 1, 1, 1, 1, 1};

  decltype(arr) out{};

  out.fill(-1);

  std::size_t n = 2;

  reduction_sweep2(arr.begin(), arr.end(), n, std::plus{}, std::identity{}, out.begin());

  CHECK(out == decltype(arr){99});

  scan_sweep(arr.begin(), arr.end(), n, std::plus{}, std::identity{}, out.begin());

  CHECK(out == decltype(arr){});
}