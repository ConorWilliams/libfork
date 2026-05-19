#include <stdio.h>
#include "test.h"

int n = 42;
int m;

static int fib_fast(int n)
{
  if (n < 2) return n;

  int i = 2, x = 0, y = 0, z = 1;

  do {
    x = y;
    y = z;
    z = x + y;
  } while (i++ < n);

  return z;
}

fibril int fib(int n)
{
  if (n < 2) return n;

  int x, y;
  fibril_t fr;
  fibril_init(&fr);

  fibril_fork(&fr, &x, fib, (n - 1));

  y = fib(n - 2);
  fibril_join(&fr);

  return x + y;
}

int verify()
{
  int expect = fib_fast(n);

  if (expect != m) {
    printf("fib(%d)=%d (expected %d)\n", n, m, expect);
    return 1;
  }

  return 0;
}

void init() {};
void prep() {};

void test() {
  m = fib(n);
}

