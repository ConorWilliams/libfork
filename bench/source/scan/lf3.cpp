// #include <algorithm>                             // for min
// #include <catch2/catch_template_test_macros.hpp> // for TEMPLATE_TEST_CASE, TypeList
// #include <catch2/catch_test_macros.hpp>          // for operator<=, operator==, INTERNAL_CATCH_...
// #include <concepts>                              // for constructible_from
// #include <cstddef>                               // for size_t
// #include <functional>                            // for plus, identity, multiplies
// #include <iostream>
// #include <iterator>
// #include <limits>  // for numeric_limits
// #include <numeric> // for inclusive_scan
// #include <random>  // for random_device, uniform_int_distribution
// #include <stdexcept>
// #include <string>      // for operator+, string, basic_string
// #include <thread>      // for thread
// #include <type_traits> // for type_identity
// #include <utility>     // for forward
// #include <vector>      // for operator==, vector

// #include <benchmark/benchmark.h>

// #include "libfork.hpp"

// #include "../util.hpp"
// #include "config.hpp"

// namespace {

// template <std::random_access_iterator I,
//           std::sized_sentinel_for<I> S,
//           class Proj,
//           class Bop,
//           std::random_access_iterator O>
// struct scanner {

//   using int_t = std::iter_difference_t<I>;
//   using acc_t = std::iter_value_t<O>;

//   LF_STATIC_CALL auto
//   operator()(auto /* */, I beg, S end, int_t n, Bop bop, Proj proj, O out) LF_STATIC_CONST->lf::task<> {

//     int_t const size = end - beg;

//     if (size <= n) {

//       acc_t acc = proj(*beg);

//       *out = acc;
//       ++beg;
//       ++out;

// #pragma unroll(8)
//       for (I last = beg + size; beg != end; ++beg, ++out) {
//         *out = acc = bop(std::move(acc), proj(*beg));
//       }

//       co_return;
//     }

//     int_t const mid = size / 2;

//     // Divide and recurse.
//     bool ready = co_await lf::fork(scanner{})(beg, beg + mid, n, bop, proj, out);

//     if (ready) {
//       co_return co_await lf::just(scanner{})(beg + mid, end, n, bop, proj, out + mid);
//     }

//     co_await lf::call(scanner{})(beg + mid, end, n, bop, proj, out + mid);
//     co_await lf::join;

//     co_await lf::just(lf::for_each)(out + mid, out + size, n, [bop, carry = *(beg + mid - 1)](auto &&elem)
//     {
//       elem = bop(carry, elem);
//     });
//   };
// };

// constexpr auto repeat = [](auto, unsigned const *in, unsigned *ou) -> lf::task<void> {
//   for (std::size_t i = 0; i < scan_reps; ++i) {
//     // std::inclusive_scan(in, in + scan_n, ou, std::plus<>{}); ///
//     co_await lf::just(scanner<unsigned const *, unsigned const *, std::identity, std::plus<>, unsigned
//     *>{})(
//         in, in + scan_n, scan_chunk, std::plus<>{}, std::identity{}, ou);
//   }
//   co_return;
// };

// template <lf::scheduler Sch, lf::numa_strategy Strategy>
// void scan_libfork3(benchmark::State &state) {

//   state.counters["green_threads"] = static_cast<double>(state.range(0));
//   state.counters["n"] = scan_n;
//   state.counters["reps"] = scan_reps;
//   state.counters["chunk"] = scan_chunk;

//   Sch sch = [&] {
//     if constexpr (std::constructible_from<Sch, int>) {
//       return Sch(state.range(0));
//     } else {
//       return Sch{};
//     }
//   }();

//   std::vector in = lf::sync_wait(sch, lf::lift, make_vec);

//   std::vector ou = lf::sync_wait(sch, lf::lift, [&] {
//     return std::vector{in};
//   });

//   volatile unsigned sink = 0;

//   for (auto _ : state) {
//     lf::sync_wait(sch, repeat, in.data(), ou.data());
//   }

//   sink = ou.back();
// }

// } // namespace

// BENCHMARK(scan_libfork3<lf::lazy_pool, lf::numa_strategy::fan>)->Apply(targs)->UseRealTime();
