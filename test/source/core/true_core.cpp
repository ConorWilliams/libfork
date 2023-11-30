#include <iostream>

#include <catch2/benchmark/catch_benchmark_all.hpp>
#include <catch2/catch_test_macros.hpp>

#define LF_DEFAULT_LOGGING

#include "libfork/core.hpp"

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

struct scheduler : lf::impl::immovable<scheduler> {

  void schedule(lf::intruded_list<lf::submit_handle> jobs) {

    context->submit(jobs);

    for_each_elem(context->try_pop_all(), [](lf::submit_handle hand) {
      resume(hand);
    });
  }

  ~scheduler() { lf::finalize(context); }

 private:
  lf::worker_context *context = lf::worker_init(1);
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

TEST_CASE("Core nooper", "[core]") {

  scheduler sch{};

  for (int i = 1; i < 20 + 1; ++i) {

    int val = lf::sync_wait(sch, fib, i);

    std::cout << "fib " << i << " = " << val << std::endl;
  }

  volatile int in = 2;

  BENCHMARK("Fibonacci serial") {
    int out;
    sfib(out, in);
    return out;
  };

  BENCHMARK("Fibonacci parall") { return lf::sync_wait(sch, call, in); };
}