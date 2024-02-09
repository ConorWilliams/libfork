

#include "extern.hpp"

LF_IMPLEMENT_NAMED(int, externed_fib, fib, int n) {

  if (n < 2) {
    co_return n;
  }

  int a = 0, b = 0;

  co_await lf::fork(&a, fib)(n - 1);
  co_await lf::call(&b, fib)(n - 2);

  co_await lf::join;

  co_return a + b;
};

LF_IMPLEMENT_NAMED(int &, externed_ref, /* unused */, int &n) { co_return n; }