/****************************************************************************\
 * LU decomposition
 * Robert Blumofe
 *
 * Copyright (c) 1996, Robert Blumofe.  All rights reserved.
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
 *
\****************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"

/* Define the size of a block. */
#ifndef BLOCK_SIZE
#define BLOCK_SIZE 16
#endif

/* Define the default matrix size. */
#ifndef DEFAULT_SIZE
#ifndef BENCHMARK
#define DEFAULT_SIZE (16 * BLOCK_SIZE)
#else
#define DEFAULT_SIZE 4096
#endif
#endif

/* A block is a 2D array of doubles. */
typedef double Block[BLOCK_SIZE][BLOCK_SIZE];
#define BLOCK(B,I,J) (B[I][J])

/* A matrix is a 1D array of blocks. */
typedef Block * Matrix;
#define MATRIX(M,I,J) ((M)[(I)*nBlocks+(J)])

/** Matrix size. */
int n = DEFAULT_SIZE;

/** The global matrix and a copy of the matrix. */
static Matrix M, Msave;

/* Matrix size in blocks. */
static int nBlocks;

/****************************************************************************\
 * Utility routines.
 \****************************************************************************/

/*
 * init_matrix - Fill in matrix M with random values.
 */
static void init_matrix(Matrix M, int nb)
{
  int I, J, K, i, j, k;

  /* Initialize random number generator. */
  srand(1);

  /* For each element of each block, fill in random value. */
  for (I = 0; I < nb; I++)
    for (J = 0; J < nb; J++)
      for (i = 0; i < BLOCK_SIZE; i++)
        for (j = 0; j < BLOCK_SIZE; j++)
          BLOCK(MATRIX(M, I, J), i, j) = ((double)rand()) / (double)RAND_MAX;

  /* Inflate diagonal entries. */
  for (K = 0; K < nb; K++)
    for (k = 0; k < BLOCK_SIZE; k++)
      BLOCK(MATRIX(M, K, K), k, k) *= 10.0;
}

/*
 * print_matrix - Print matrix M.
 */
static void print_matrix(Matrix M, int nb)
{
  int i, j;

  /* Print out matrix. */
  for (i = 0; i < nb * BLOCK_SIZE; i++) {
    for (j = 0; j < nb * BLOCK_SIZE; j++)
      printf(" %6.4f",
          BLOCK(MATRIX(M, i / BLOCK_SIZE, j / BLOCK_SIZE),
            i % BLOCK_SIZE, j % BLOCK_SIZE));
    printf("\n");
  }
}

/*
 * test_result - Check that matrix LU contains LU decomposition of M.
 */
static int test_result(Matrix LU, Matrix M, int nb)
{
  int I, J, K, i, j, k;
  double diff, max_diff;
  double v;

  /* Initialize test. */
  max_diff = 0.0;

  /* Find maximum difference between any element of LU and M. */
  for (i = 0; i < nb * BLOCK_SIZE; i++)
    for (j = 0; j < nb * BLOCK_SIZE; j++) {
      I = i / BLOCK_SIZE;
      J = j / BLOCK_SIZE;
      v = 0.0;
      for (k = 0; k < i && k <= j; k++) {
        K = k / BLOCK_SIZE;
        v += BLOCK(MATRIX(LU, I, K), i % BLOCK_SIZE,
            k % BLOCK_SIZE) *
          BLOCK(MATRIX(LU, K, J), k % BLOCK_SIZE,
              j % BLOCK_SIZE);
      }
      if (k == i && k <= j) {
        K = k / BLOCK_SIZE;
        v += BLOCK(MATRIX(LU, K, J), k % BLOCK_SIZE,
            j % BLOCK_SIZE);
      }
      diff = fabs(BLOCK(MATRIX(M, I, J), i % BLOCK_SIZE,
            j % BLOCK_SIZE) - v);
      if (diff > max_diff)
        max_diff = diff;
    }

  /* Check maximum difference against threshold. */
  return (max_diff > 0.00001);
}

/****************************************************************************\
 * Element operations.
 \****************************************************************************/
/*
 * elem_daxmy - Compute y' = y - ax where a is a double and x and y are
 * vectors of doubles.
 */
static void elem_daxmy(double a, double *x, double *y, int n)
{
  for (n--; n >= 0; n--) y[n] -= a * x[n];
}

/****************************************************************************\
 * Block operations.
 \****************************************************************************/

/*
 * block_lu - Factor block B.
 */
static void block_lu(Block B)
{
  int i, k;

  /* Factor block. */
  for (k = 0; k < BLOCK_SIZE; k++)
    for (i = k + 1; i < BLOCK_SIZE; i++) {
      BLOCK(B, i, k) /= BLOCK(B, k, k);
      elem_daxmy(BLOCK(B, i, k), &BLOCK(B, k, k + 1),
          &BLOCK(B, i, k + 1), BLOCK_SIZE - k - 1);
    }
}

/*
 * block_lower_solve - Perform forward substitution to solve for B' in
 * LB' = B.
 */
static void block_lower_solve(Block B, Block L)
{
  int i, k;

  /* Perform forward substitution. */
  for (i = 1; i < BLOCK_SIZE; i++)
    for (k = 0; k < i; k++)
      elem_daxmy(BLOCK(L, i, k), &BLOCK(B, k, 0),
          &BLOCK(B, i, 0), BLOCK_SIZE);
}

/*
 * block_upper_solve - Perform forward substitution to solve for B' in
 * B'U = B.
 */
static void block_upper_solve(Block B, Block U)
{
  int i, k;

  /* Perform forward substitution. */
  for (i = 0; i < BLOCK_SIZE; i++)
    for (k = 0; k < BLOCK_SIZE; k++) {
      BLOCK(B, i, k) /= BLOCK(U, k, k);
      elem_daxmy(BLOCK(B, i, k), &BLOCK(U, k, k + 1),
          &BLOCK(B, i, k + 1), BLOCK_SIZE - k - 1);
    }
}

/*
 * block_schur - Compute Schur complement B' = B - AC.
 */
static void block_schur(Block B, Block A, Block C)
{
  int i, k;

  /* Compute Schur complement. */
  for (i = 0; i < BLOCK_SIZE; i++)
    for (k = 0; k < BLOCK_SIZE; k++)
      elem_daxmy(BLOCK(A, i, k), &BLOCK(C, k, 0),
          &BLOCK(B, i, 0), BLOCK_SIZE);
}


/****************************************************************************\
 * Divide-and-conquer matrix LU decomposition.
 \****************************************************************************/

/**
 * schur - Compute M' = M - VW.
 */
fibril static void schur(Matrix M, Matrix V, Matrix W, int nb)
{
  Matrix M00, M01, M10, M11;
  Matrix V00, V01, V10, V11;
  Matrix W00, W01, W10, W11;
  int hnb;

  /* Check base case. */
  if (nb == 1) {
    block_schur(*M, *V, *W);
    return;
  }

  /* Break matrices into 4 pieces. */
  hnb = nb / 2;
  M00 = &MATRIX(M, 0, 0);
  M01 = &MATRIX(M, 0, hnb);
  M10 = &MATRIX(M, hnb, 0);
  M11 = &MATRIX(M, hnb, hnb);
  V00 = &MATRIX(V, 0, 0);
  V01 = &MATRIX(V, 0, hnb);
  V10 = &MATRIX(V, hnb, 0);
  V11 = &MATRIX(V, hnb, hnb);
  W00 = &MATRIX(W, 0, 0);
  W01 = &MATRIX(W, 0, hnb);
  W10 = &MATRIX(W, hnb, 0);
  W11 = &MATRIX(W, hnb, hnb);

  /* Form Schur complement with recursive calls. */
  fibril_t fr;
  fibril_init(&fr);

  fibril_fork(&fr, schur, (M00, V00, W00, hnb));
  fibril_fork(&fr, schur, (M01, V00, W01, hnb));
  fibril_fork(&fr, schur, (M10, V10, W00, hnb));
  schur(M11, V10, W01, hnb);
  fibril_join(&fr);

  fibril_fork(&fr, schur, (M00, V01, W10, hnb));
  fibril_fork(&fr, schur, (M01, V01, W11, hnb));
  fibril_fork(&fr, schur, (M10, V11, W10, hnb));
  schur(M11, V11, W11, hnb);
  fibril_join(&fr);

  return;
}

/*
 * lower_solve - Compute M' where LM' = M.
 */
fibril static void lower_solve(Matrix M, Matrix L, int nb);

static void aux_lower_solve(Matrix Ma, Matrix Mb, Matrix L, int nb)
{
  Matrix L00, L01, L10, L11;

  /* Break L matrix into 4 pieces. */
  L00 = &MATRIX(L, 0, 0);
  L01 = &MATRIX(L, 0, nb);
  L10 = &MATRIX(L, nb, 0);
  L11 = &MATRIX(L, nb, nb);

  /* Solve with recursive calls. */
  lower_solve(Ma, L00, nb);
  schur(Mb, L10, Ma, nb);
  lower_solve(Mb, L11, nb);
}

fibril static void lower_solve(Matrix M, Matrix L, int nb)
{
  Matrix M00, M01, M10, M11;
  int hnb;

  /* Check base case. */
  if (nb == 1) {
    block_lower_solve(*M, *L);
    return;
  }

  /* Break matrices into 4 pieces. */
  hnb = nb / 2;
  M00 = &MATRIX(M, 0, 0);
  M01 = &MATRIX(M, 0, hnb);
  M10 = &MATRIX(M, hnb, 0);
  M11 = &MATRIX(M, hnb, hnb);

  /* Solve with recursive calls. */
  fibril_t fr;
  fibril_init(&fr);

  fibril_fork(&fr, aux_lower_solve, (M00, M10, L, hnb));
  aux_lower_solve(M01, M11, L, hnb);

  fibril_join(&fr);

  return;
}

/*
 * upper_solve - Compute M' where M'U = M.
 */
fibril static void upper_solve(Matrix M, Matrix U, int nb);

static void aux_upper_solve(Matrix Ma, Matrix Mb, Matrix U, int nb)
{
  Matrix U00, U01, U10, U11;

  /* Break U matrix into 4 pieces. */
  U00 = &MATRIX(U, 0, 0);
  U01 = &MATRIX(U, 0, nb);
  U10 = &MATRIX(U, nb, 0);
  U11 = &MATRIX(U, nb, nb);

  /* Solve with recursive calls. */
  upper_solve(Ma, U00, nb);
  schur(Mb, Ma, U01, nb);
  upper_solve(Mb, U11, nb);

  return;
}

fibril static void upper_solve(Matrix M, Matrix U, int nb)
{
  Matrix M00, M01, M10, M11;
  int hnb;

  /* Check base case. */
  if (nb == 1) {
    block_upper_solve(*M, *U);
    return;
  }

  /* Break matrices into 4 pieces. */
  hnb = nb / 2;
  M00 = &MATRIX(M, 0, 0);
  M01 = &MATRIX(M, 0, hnb);
  M10 = &MATRIX(M, hnb, 0);
  M11 = &MATRIX(M, hnb, hnb);

  /* Solve with recursive calls. */
  fibril_t fr;
  fibril_init(&fr);

  fibril_fork(&fr, aux_upper_solve, (M00, M01, U, hnb));
  aux_upper_solve(M10, M11, U, hnb);

  fibril_join(&fr);

  return;
}

/*
 * lu - Perform LU decomposition of matrix M.
 */
fibril void lu(Matrix M, int nb)
{
  Matrix M00, M01, M10, M11;
  int hnb;

  /* Check base case. */
  if (nb == 1) {
    block_lu(*M);
    return;
  }

  /* Break matrix into 4 pieces. */
  hnb = nb / 2;
  M00 = &MATRIX(M, 0, 0);
  M01 = &MATRIX(M, 0, hnb);
  M10 = &MATRIX(M, hnb, 0);
  M11 = &MATRIX(M, hnb, hnb);

  /* Decompose upper left. */
  lu(M00, hnb);

  /* Solve for upper right and lower left. */
  fibril_t fr;
  fibril_init(&fr);

  fibril_fork(&fr, lower_solve, (M01, M00, hnb));
  upper_solve(M10, M00, hnb);

  fibril_join(&fr);

  /* Compute Schur complement of lower right. */
  schur(M11, M10, M01, hnb);

  /* Decompose lower right. */
  lu(M11, hnb);

  return;
}

void init()
{
  nBlocks = n / BLOCK_SIZE;
  M = (Matrix) malloc(n * n * sizeof(double));
  init_matrix(M, nBlocks);
#ifndef BENCHMARK
  Msave = (Matrix) malloc(n * n * sizeof(double));
  memcpy((void *) Msave, (void *) M, n * n * sizeof(double));
#endif

}

void prep()
{
#ifndef BENCHMARK
  memcpy((void *) M, (void *) Msave, n * n * sizeof(double));
#endif
}

void test()
{
  lu(M, nBlocks);
}

int verify()
{
#ifndef BENCHMARK
  return test_result(M, Msave, nBlocks);
#else
  return 0;
#endif
}
