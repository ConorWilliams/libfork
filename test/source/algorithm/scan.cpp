
#include <functional>
#include <iostream>
#include <iterator>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/algorithm/constraints.hpp"
#include "libfork/algorithm/scan.hpp"
#include "libfork/core.hpp"
#include "libfork/core/sync_wait.hpp"
#include "libfork/schedule.hpp"

using namespace lf;

namespace {

// TODO: test a non commutative operation.

/**
 *
 * How we invoke Bop:
 *
 *  bop(*out, proj(*in))
 *  bop(*out, std::ranges::iter_move(out))
 *
 *  *out = proj(*in)
 *  *out = bop(...);
 *
 */

/**
 * @brief Out of place.
 */
template <typename I, typename S, typename O, typename Bop, typename Proj>
auto all_up_scan(I beg, S end, std::iter_difference_t<I> n, Bop bop, Proj proj, O out) -> void {

  if (auto size = end - beg; size <= n) {
    // Assignable/writable from projected input (std::indirectly_copyable?)
    *out = proj(*beg);

    for (++beg; beg != end; ++beg) {
      // Assignable/writable from bop result.
      // Bop is a common_semigroup over projected iterator and the output iterator.
      auto prev = out;
      ++out;
      *out = bop(*prev, proj(*beg));
    }
  } else {
    // Recurse to smaller chunks.
    auto half = size / 2;
    auto mid = beg + half;
    all_up_scan(beg, mid, n, bop, proj, out);
    all_up_scan(mid, end, n, bop, proj, out + half);

    // Accumulate in rhs of output chunk.
    // Require bop is a common_semigroup over output iter with with l and r value refs.
    auto l_child = out + (half - 1);
    auto r_child = out + (size - 1);
    *r_child = bop(*l_child, std::ranges::iter_move(r_child));
  }
}

/**
 * @brief Operates in place.
 */
template <typename I, typename S, typename Bop>
auto rhs_down_scan(I beg, S end, std::iter_difference_t<I> n, Bop bop) -> void {

  /**
   * Chunks looks like:
   *
   *  [a, b, c, acc_prev] [d, e, f, acc] [h, i, j, acc_next]
   *                       ^- beg         ^- end
   */

  auto acc_prev = beg - 1; // Carried/previous accumulation

  if (auto size = end - beg; size <= n) {
    for (; beg != end - 1; ++beg) {
      // Same as all_up_scan's second assignment.
      *beg = bop(*acc_prev, std::ranges::iter_move(beg));
    }
  } else {
    auto half = size / 2;
    auto mid = beg + half;

    auto rhs = beg + (half - 1);                        // This is the left child of the tree.
    *rhs = bop(*acc_prev, std::ranges::iter_move(rhs)); // Same as all_up_scan's second assignment.

    rhs_down_scan(beg, mid, n, bop);
    rhs_down_scan(mid, end, n, bop);
  }
}

template <typename I, typename S, typename Bop>
auto lhs_down_scan(I beg, S end, std::iter_difference_t<I> n, Bop bop) -> void {
  if (auto size = end - beg; size > n) {
    auto mid = beg + size / 2;
    lhs_down_scan(beg, mid, n, bop);
    rhs_down_scan(mid, end, n, bop);
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
template <typename I, typename S, typename O, typename Bop, typename Proj = std::identity>
auto scan(I beg, S end, O out, std::iter_difference_t<I> n, Bop bop, Proj proj = {}) -> void {
  if (auto size = end - beg; size > 0) {
    all_up_scan(beg, end, n, bop, proj, out);
    lhs_down_scan(out, out + size, n, bop);
  }
}

template <typename T>
auto make_scheduler() -> T {
  if constexpr (std::constructible_from<T, std::size_t>) {
    return T{std::min(4U, std::thread::hardware_concurrency())};
  } else {
    return T{};
  }
}

} // namespace

/**
 * @brief Test matrix:
 *
 * 1. in place vs out of place
 * 2. commutative vs non commutative
 * 3. chunk size
 * 4. input size
 * 5. projections (or lack thereof)
 */

TEMPLATE_TEST_CASE("scan", "[algorithm][template]", unit_pool, busy_pool, lazy_pool) {

  auto sch = make_scheduler<TestType>();

  for (int chunk = 1; chunk <= 15; chunk++) {

    // std::cout << "chunk: " << chunk << std::endl;

    for (int n = 1; n <= 15; n++) {

      std::vector<int> v(n, 1);
      std::vector<int> out(v.size());

      // inclusive_scan(v.begin(), v.end(), out.begin());

      // scan(v.begin(), v.end(), out.begin(), chunk, std::plus<>{});

      sync_wait(sch, lf::impl::scan_overload{}, v.begin(), v.end(), out.begin(), chunk, std::plus<>{});

      // for (auto &&elem : out) {
      //   std::cout << elem << " ";
      // }
      // std::cout << std::endl;

      for (int i = 0; i < v.size(); i++) {
        REQUIRE(out[i] == i + 1);
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
