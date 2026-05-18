/*
 * Copyright (c) 2000 Massachusetts Institute of Technology
 * Copyright (c) 2000 Matteo Frigo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "test.h"
#include "fft.h"

#ifdef BENCHMARK
int n = 26;
#else
int n = 12;
#endif

static int size;
static COMPLEX *in, *out, *cp, *W;
static const REAL pi = 3.1415926535897932384626434;

/*
 * compute the W coefficients (that is, powers of the root of 1)
 * and store them into an array.
 */
fibril static void compute_w_coefficients(int n, int a, int b, COMPLEX * W)
{
  register double twoPiOverN;
  register int k;
  register REAL s, c;

  if (b - a < 128) {
    twoPiOverN = 2.0 * pi / n;
    for (k = a; k <= b; ++k) {
      c = cos(twoPiOverN * k);
      c_re(W[k]) = c_re(W[n - k]) = c;
      s = sin(twoPiOverN * k);
      c_im(W[k]) = -s;
      c_im(W[n - k]) = s;
    }
  } else {
    int ab = (a + b) / 2;

    fibril_t fr;
    fibril_init(&fr);

    fibril_fork(&fr, compute_w_coefficients, (n, a, ab, W));
    compute_w_coefficients(n, ab + 1, b, W);

    fibril_join(&fr);
  }
}

/*
 * Determine (in a stupid way) if n is divisible by eight, then by four, else
 * find the smallest prime factor of n.
 */
static int factor(int n)
{
  int r;

  if (n < 2)
    return 1;

  if (n == 64 || n == 128 || n == 256 || n == 1024 || n == 2048
      || n == 4096)
    return 8;
  if ((n & 15) == 0)
    return 16;
  if ((n & 7) == 0)
    return 8;
  if ((n & 3) == 0)
    return 4;
  if ((n & 1) == 0)
    return 2;

#if 0
  /* radix-32 is too big --- wait for processors with more registers
   * :-) */
  if ((n & 31) == 0 && n > 256)
    return 32;
#endif

  /* try odd numbers up to n (computing the sqrt may be slower) */
  for (r = 3; r < n; r += 2)
    if (n % r == 0)
      return r;

  /* n is prime */
  return n;
}

fibril static void unshuffle(int a, int b,
    COMPLEX * in, COMPLEX * out, int r, int m)
{
  int i, j;
  int r4 = r & (~0x3);
  const COMPLEX *ip;
  COMPLEX *jp;

  if (b - a < 16) {
    ip = in + a * r;
    for (i = a; i < b; ++i) {
      jp = out + i;
      for (j = 0; j < r4; j += 4) {
        jp[0] = ip[0];
        jp[m] = ip[1];
        jp[2 * m] = ip[2];
        jp[3 * m] = ip[3];
        jp += 4 * m;
        ip += 4;
      }
      for (; j < r; ++j) {
        *jp = *ip;
        ip++;
        jp += m;
      }
    }
  } else {
    int ab = (a + b) / 2;

    fibril_t fr;
    fibril_init(&fr);

    fibril_fork(&fr, unshuffle, (a, ab, in, out, r, m));
    unshuffle(ab, b, in, out, r, m);

    fibril_join(&fr);
  }
}

/*
 * Recursive complex FFT on the n complex components of the array in:
 * basic Cooley-Tukey algorithm, with some improvements for
 * n power of two. The result is placed in the array out. n is arbitrary.
 * The algorithm runs in time O(n*(r1 + ... + rk)) where r1, ..., rk
 * are prime numbers, and r1 * r2 * ... * rk = n.
 *
 * n: size of the input
 * in: pointer to input
 * out: pointer to output
 * factors: list of factors of n, precomputed
 * W: twiddle factors
 * nW: size of W, that is, size of the original transform
 *
 */
fibril static void fft_aux(int n, COMPLEX * in, COMPLEX * out, int *factors,
    COMPLEX * W, int nW)
{
  int r, m;

  /* special cases */
  if (n == 32) {
    fft_base_32(in, out);
    return;
  }
  if (n == 16) {
    fft_base_16(in, out);
    return;
  }
  if (n == 8) {
    fft_base_8(in, out);
    return;
  }
  if (n == 4) {
    fft_base_4(in, out);
    return;
  }
  if (n == 2) {
    fft_base_2(in, out);
    return;
  }
  /* the cases n == 3, n == 5, and maybe 7 should be implemented as well */

  r = *factors;
  m = n / r;

  if (r < n) {
    /* split the DFT of length n into r DFTs of length n/r,  and recurse */
    if (r == 32)
      fft_unshuffle_32(0, m, in, out, m);
    else if (r == 16)
      fft_unshuffle_16(0, m, in, out, m);
    else if (r == 8)
      fft_unshuffle_8(0, m, in, out, m);
    else if (r == 4)
      fft_unshuffle_4(0, m, in, out, m);
    else if (r == 2)
      fft_unshuffle_2(0, m, in, out, m);
    else
      unshuffle(0, m, in, out, r, m);

    fibril_t fr;
    fibril_init(&fr);

    int k;
    for(k = 0; k < n; k += m) {
      fibril_fork(&fr, fft_aux, (m, out + k, in + k, factors + 1, W, nW));
    }

    fibril_join(&fr);
  }

  /* now multiply by the twiddle factors, and perform m FFTs of length r */
  if (r == 2)
    fft_twiddle_2(0, m, in, out, W, nW, nW / n, m);
  else if (r == 4)
    fft_twiddle_4(0, m, in, out, W, nW, nW / n, m);
  else if (r == 8)
    fft_twiddle_8(0, m, in, out, W, nW, nW / n, m);
  else if (r == 16)
    fft_twiddle_16(0, m, in, out, W, nW, nW / n, m);
  else if (r == 32)
    fft_twiddle_32(0, m, in, out, W, nW, nW / n, m);
  else
    fft_twiddle_gen(0, m, in, out, W, nW, nW / n, r, m);

  return;
}

/*
 * user interface for fft_aux
 */
static void fft(int n, COMPLEX * in, COMPLEX * out)
{
  int factors[40];		/* allows FFTs up to at least 3^40 */
  int *p = factors;
  int l = n;
  int r;

  compute_w_coefficients(n, 0, n / 2, W);

  /**
   * find factors of n, first 8, then 4 and then primes in ascending
   * order.
   */
  do {
    r = factor(l);
    *p++ = r;
    l /= r;
  } while (l > 1);

  fft_aux(n, in, out, factors, W, n);
  return;
}

/****************************************************************
 *                     END OF FFT ALGORITHM
 ****************************************************************/

/*                            tests                             */

static void fft_alt(int n, COMPLEX * in, COMPLEX * out)
{
  int i, j;
  COMPLEX sum;
  COMPLEX w;

  for (j = 0; j < n; ++j) {
    c_re(sum) = c_im(sum) = 0.0;

    for (i = 0; i < n; ++i) {
      c_re(w) = cos((2.0 * pi * (i * j % n)) / n);
      c_im(w) = -sin((2.0 * pi * (i * j % n)) / n);
      c_re(sum) += c_re(in[i]) * c_re(w) - c_im(in[i]) * c_im(w);
      c_im(sum) += c_im(in[i]) * c_re(w) + c_re(in[i]) * c_im(w);
    }

    out[j] = sum;
  }

  return;
}

void init()
{
  size = (1 << n);
  out = malloc(sizeof(COMPLEX [size]));
  in  = malloc(sizeof(COMPLEX [size]));
  W   = malloc(sizeof(COMPLEX [size + 1]));

  int i;
  for (i = 0; i < size; ++i) {
    c_re(in[i]) = drand48();
    c_im(in[i]) = drand48();
  }
}

void prep()
{
  if (cp == NULL)
    cp = malloc(sizeof(COMPLEX [size]));

  memcpy(cp, in, sizeof(COMPLEX [size]));
}

void test()
{
  fft(size, cp, out);
}

#ifdef BENCHMARK
int verify(void) { return 0; }
#else
int verify(void)
{
  COMPLEX * expect = malloc(sizeof(COMPLEX [size]));

  fft_alt(size, in, expect);

  /* compute the relative error */
  double error = 0.0;

  int i;
  for (i = 0; i < size; ++i) {
    double a = sqrt(
        (c_re(out[i]) - c_re(expect[i])) * (c_re(out[i]) - c_re(expect[i])) +
        (c_im(out[i]) - c_im(expect[i])) * (c_im(out[i]) - c_im(expect[i])));
    double d = sqrt(
        c_re(expect[i]) * c_re(expect[i]) + c_im(expect[i]) * c_im(expect[i]));

    if (d < -1.0e-10 || d > 1.0e-10) a /= d;
    if (a > error) error = a;
  }

  if (error > 1e-3) {
    printf("size=%d error=%e\n", size, error);
    return 1;
  } else {
    return 0;
  }
}
#endif

