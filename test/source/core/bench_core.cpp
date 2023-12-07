#include <iostream>

#include <catch2/benchmark/catch_benchmark_all.hpp>
#include <catch2/catch_test_macros.hpp>

// #define LF_DEFAULT_LOGGING

#include "libfork/core.hpp"

namespace {

inline constexpr auto noop = [](lf::first_arg auto noop) -> lf::task<> {
  co_return;
};

inline constexpr auto call = [](auto, int n) -> lf::task<int> {
  co_return n;
};

struct noise {
  noise() { std::cout << "cons()" << std::endl; }
  ~noise() { std::cout << "dest()" << std::endl; }
};

inline constexpr auto fib = [](auto fib, int n) -> lf::task<int> {
  //
  // noise _;

  if (n < 2) {
    co_return n;
  }

  int a, b;

  co_await lf::fork(&a, fib)(n - 1);
  co_await lf::call(&b, fib)(n - 2);

  co_return a + b;
};

inline constexpr auto co_fib = [](auto co_fib, int n) -> lf::task<int> {
  if (n < 2) {
    co_return n;
  }

  std::span r = co_await lf::co_new<int, 2>();

  co_await lf::fork(&r[0], co_fib)(n - 1);
  co_await lf::call(&r[1], co_fib)(n - 2);

  co_await lf::join;

  int res = r[1] + r[0];

  co_await lf::co_delete(r);

  co_return res;
};

struct scheduler : lf::impl::immovable<scheduler> {

  void schedule(lf::intruded_list<lf::submit_handle> jobs) {

    context->submit(jobs);

    for_each_elem(context->try_pop_all(), [](lf::submit_handle hand) {
      resume(hand);
    });
  }

  ~scheduler() { lf::finalize(context); }

 private:
  lf::worker_context *context = lf::worker_init(lf::nullary_function_t{[]() {}});
};

LF_NOINLINE constexpr auto sfib(int &ret, int n) -> void {
  if (n < 2) {
    ret = n;
    return;
  }

  int a, b;

  sfib(a, n - 1);
  sfib(b, n - 2);

  ret = a + b;
};

auto test(auto) -> lf::task<> { co_return {}; }

} // namespace

// TEST_CASE("Core nooper", "[core][benchmark]") {

//   scheduler sch{};

//   for (int i = 1; i < 30 + 1; ++i) {

//     int val = lf::sync_wait(sch, fib, i);

//     std::cout << "fib " << i << " = " << val << std::endl;
//   }

//   volatile int in = 32;

//   BENCHMARK("Fibonacci serial") {
//     int out;
//     sfib(out, in);
//     return out;
//   };

//   BENCHMARK("Fibonacci parall") {
//     //
//     return lf::sync_wait(sch, fib, in);
//   };

//   BENCHMARK("Fibonacci parall co_alloc") {
//     //
//     return lf::sync_wait(sch, co_fib, in);
//   };
// }
