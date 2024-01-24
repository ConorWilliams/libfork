
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

// for (auto it = ++beg; it != end; ++it) {
//   auto acc = out;
//   ++out;
//   *out = *acc + *it;
// }

// inline constexpr int chunk = 3;

template <typename T>
auto bop(T beg, T right) -> T {
  *right = *beg + *right;
  return right;
}

template <typename T>
auto scan_up(T beg, T end) -> T {
  switch (auto size = end - beg) {
    case 1:
      return beg; // This is the left and right child.
    case 2:
      return bop(beg, beg + 1); // Returns right child.
    default:
      auto half = beg + size / 2;

      auto left1 = scan_up(beg, half);
      auto right = scan_up(half, end);

      return bop(left1, right); // Returns right child.
  }
}

template <typename T>
auto scan_down(T beg, T end, auto node) {

  /**
   * Pattern
   *
   * tmp <- right (node)
   * right <- node
   * beg <- tmp + node
   */

  switch (auto size = end - beg) {
    case 1:
      return;
    case 2: {
      *(beg + 1) = std::ranges::iter_move(beg) + node;
      *beg = std::move(node);
      return;
    }
    default:
      auto half = beg + size / 2;

      auto left = beg + size / 2 - 1;

      auto tmp1 = node + *left;

      scan_down(beg, half, std::move(node)); // Left recursion
      scan_down(half, end, std::move(tmp1)); // Right recursion
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
auto scan(T beg, T end) {
  scan_up(beg, end);
  scan_down(beg, end, 0);
}

} // namespace

TEMPLATE_TEST_CASE("scan", "[algorithm][template]", unit_pool /*, busy_pool, lazy_pool*/) {

  for (int n = 1; n < 10; n++) {

    std::vector<int> v(n, 1);
    std::vector<int> out;

    scan(v.begin(), v.end());

    for (auto &&elem : out) {
      std::cout << elem << " ";
    }

    // std::cout << std::endl;

    // for (int i = 0; i < v.size(); i++) {
    CHECK(v == out);
    // }
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
