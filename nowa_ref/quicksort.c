#include <math.h>
#include <stdlib.h>
#include "test.h"

int n = 8;
static int * a, * b;
static size_t size;

fibril void quicksort(int * a, size_t n)
{
  if (n < 2) return;

  int pivot = a[n / 2];

  int *left  = a;
  int *right = a + n - 1;

  while (left <= right) {
    if (*left < pivot) {
      left++;
    } else if (*right > pivot) {
      right--;
    } else {
      int tmp = *left;
      *left = *right;
      *right = tmp;
      left++;
      right--;
    }
  }

  fibril_t fr;
  fibril_init(&fr);

  fibril_fork(&fr, quicksort, (a, right - a + 1));
  quicksort(left, a + n - left);

  fibril_join(&fr);
}

int verify()
{
  if (size < 2) return 0;

  int prev = a[0];
  int i;
  for (i = 1; i < size; ++i) {
    if (prev > a[i]) return 1;
    prev = a[i];
  }

  return 0;
}

void init()
{
  size = 1;

  int i;
  for (i = 0; i < n; ++i) {
    size *= 10;
  }

  a = malloc(sizeof(int [size]));
  b = malloc(sizeof(int [size]));

  for (i = 0; i < size; ++i) {
    b[i] = rand();
  }
}

void prep()
{
  int i;
  for (i = 0; i < size; ++i) {
    a[i] = b[i];
  }
}

void test()
{
  quicksort(a, size);
}

