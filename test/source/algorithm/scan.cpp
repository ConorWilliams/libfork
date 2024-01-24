
#include <functional>
#include <iostream>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iterator>

#include "libfork/algorithm/scan.hpp"
#include "libfork/core.hpp"
#include "libfork/schedule.hpp"

using namespace lf;

namespace {

// TODO: test a non commutative operation.

/**
 * @brief Out of place.
 */
template <typename I, typename S, typename O, typename Bop, typename Proj>
auto up_scan(I beg, S end, std::iter_difference_t<I> n, Bop bop, Proj proj, O out) -> void {

  if (auto size = end - beg; size <= n) {

    // Assignable/writable from projected input (std::indirectly_copyable?)
    *out = proj(*beg);

    for (++beg; beg != end; ++beg) {
      auto prev = out;
      // Assignable/writable from bop result.
      // Bop is a common_semigroup over projected iterator and the output iterator.
      *++out = bop(*prev, proj(*beg));
    }
  } else {
    // Recurse to smaller chunks.
    auto half = size / 2;
    auto mid = beg + half;
    up_scan(beg, mid, n, bop, proj, out);
    up_scan(mid, end, n, bop, proj, out + half);

    // TODO: give these the correct names l_child, r_child etc.

    // Accumulate in rhs of output chunk.
    auto lhs = out + (half - 1);
    auto rhs = out + (size - 1);
    // Bop is a common_semigroup over output iter with with l and r value refs.
    *rhs = bop(*lhs, std::ranges::iter_move(rhs));
  }
}

/**
 * @brief Operates in place.
 */
template <typename I, typename S, typename Bop>
auto down_scan_r(I beg, S end, std::iter_difference_t<I> n, Bop bop) -> void {

  if (auto size = end - beg; size <= n) {
    /**
     * Chunks looks like:
     *
     *  [a, b, c, acc_1] [d, e, f, acc_2] [h, i, j, acc_3]
     *                    ^- beg           ^- end
     */

    auto acc = beg - 1; // Carried/previous accumulation

    for (; beg != end - 1; ++beg) {
      // Same as up_scan's second assignment.
      *beg = bop(*acc, std::ranges::iter_move(beg));
    }
  } else {
    auto half = size / 2;
    auto mid = beg + half;

    auto acc = beg - 1;                            // Carried/previous accumulation
    auto rhs = beg + (half - 1);                   // This is the left child of the tree.
    *rhs = bop(*acc, std::ranges::iter_move(rhs)); // Same as up_scan's second assignment.

    down_scan_r(beg, mid, n, bop);
    down_scan_r(mid, end, n, bop);
  }
}

template <typename I, typename S, typename Bop>
auto scan_down_l(I beg, S end, auto n, Bop bop) -> void {
  if (auto size = end - beg; size > n) {
    auto mid = beg + size / 2;
    scan_down_l(beg, mid, n, bop);
    down_scan_r(mid, end, n, bop);
  }
}

/**
 * @brief
 *
 * y0 = x0
 * y1 = x0 + x1
 * y2 = x0 + x1+ x2
 *
 */
template <typename T>
auto scan(T beg, T end, auto n) {
  up_scan(beg, end, n, std::plus<>{}, std::identity{}, beg);
  scan_down_l(beg, end, n, std::plus<>{});
}

} // namespace

TEMPLATE_TEST_CASE("scan", "[algorithm][template]", unit_pool /*, busy_pool, lazy_pool*/) {

  for (int chunk = 1; chunk <= 15; chunk++) {

    // std::cout << "chunk: " << chunk << std::endl;

    for (int n = 1; n <= 15; n++) {

      std::vector<int> v(n, 1);
      std::vector<int> out;

      scan(v.begin(), v.end(), chunk);

      // for (auto &&elem : v) {
      //   std::cout << elem << " ";
      // }
      // std::cout << std::endl;

      for (int i = 0; i < v.size(); i++) {
        REQUIRE(v[i] == i + 1);
      }
    }
  }

  // for (int n = 1; n < 10; n++) {

  //   std::vector<int> v(n, 1);
  //   std::vector<int> out(v.size());

  //   scan<4>(v.begin(), v.end(), out.begin());

  //   for (auto &&elem : out) {
  //     std::cout << elem << " ";
  //   }

  //   std::cout << std::endl;

  //   for (int i = 0; i < v.size(); i++) {
  //     REQUIRE(out[i] == i + 1);
  //   }
  // }
}
