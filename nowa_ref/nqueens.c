#include <stdio.h>
#include "test.h"

int n = 14;
int m;

fibril static int nqueens(const int * a, int n, int d, int i)
{
  int aa[d + 1];
  int j;

  for (j = 0; j < d; ++j) {
    aa[j] = a[j];

    int diff = a[j] - i;
    int dist = d - j;

    if (diff == 0 || dist == diff || dist + diff == 0) return 0;
  }

  if (d >= 0) aa[d] = i;
  if (++d == n) return 1;

  int res[n];
  a = aa;

  fibril_t fr;
  fibril_init(&fr);

  for (i = 0; i < n; ++i) {
    fibril_fork(&fr, &res[i], nqueens, (a, n, d, i));
  }

  fibril_join(&fr);

  int sum = 0;

  for (i = 0; i < n; ++i) {
    sum += res[i];
  }

  return sum;
}

void init() {}
void prep() {}

void test()
{
  m = nqueens(NULL, n, -1, 0);
}

int verify()
{
  static int res[16] = {
    1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 2680,
    14200, 73712, 365596, 2279184, 14772512
  };

  int failed;

  if (failed = (m != res[n - 1])) {
      fprintf(stderr, "nqueens(%d)=%d (expected %d)\n", n, m, res[n - 1]);
  }

  return failed;
}

