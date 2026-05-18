/*
 * Program to multiply two rectangualar matrizes A(n,m) * B(m,n), where
 * (n < m) and (n mod 16 = 0) and (m mod n = 0). (Otherwise fill with 0s
 * to fit the shape.)
 *
 * written by Harald Prokop (prokop@mit.edu) Fall 97.
 */
/*
 * Copyright (c) 2003 Massachusetts Institute of Technology
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
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "test.h"

#define BLOCK_EDGE 16
#define BLOCK_SIZE (BLOCK_EDGE * BLOCK_EDGE)

typedef double block[BLOCK_SIZE];

#ifndef BENCHMARK
int n = 512;
#else
int n = 4096;
#endif

static block * A, * B, * R;
static int x, y, z;

/* compute R = R+AB, where R,A,B are BLOCK_EDGE x BLOCK_EDGE matricies
*/
static void mult_add_block(block * A, block * B, block * R)
{
  int i, j;

  for (j = 0; j < 16; j += 2) {	/* 2 columns at a time */
    double *bp = &((double *) B)[j];
    for (i = 0; i < 16; i += 2) {		/* 2 rows at a time */
      double *ap = &((double *) A)[i * 16];
      double *rp = &((double *) R)[j + i * 16];
      register double s0_0, s0_1;
      register double s1_0, s1_1;
      s0_0 = rp[0];
      s0_1 = rp[1];
      s1_0 = rp[16];
      s1_1 = rp[17];
      s0_0 += ap[0] * bp[0];
      s0_1 += ap[0] * bp[1];
      s1_0 += ap[16] * bp[0];
      s1_1 += ap[16] * bp[1];
      s0_0 += ap[1] * bp[16];
      s0_1 += ap[1] * bp[17];
      s1_0 += ap[17] * bp[16];
      s1_1 += ap[17] * bp[17];
      s0_0 += ap[2] * bp[32];
      s0_1 += ap[2] * bp[33];
      s1_0 += ap[18] * bp[32];
      s1_1 += ap[18] * bp[33];
      s0_0 += ap[3] * bp[48];
      s0_1 += ap[3] * bp[49];
      s1_0 += ap[19] * bp[48];
      s1_1 += ap[19] * bp[49];
      s0_0 += ap[4] * bp[64];
      s0_1 += ap[4] * bp[65];
      s1_0 += ap[20] * bp[64];
      s1_1 += ap[20] * bp[65];
      s0_0 += ap[5] * bp[80];
      s0_1 += ap[5] * bp[81];
      s1_0 += ap[21] * bp[80];
      s1_1 += ap[21] * bp[81];
      s0_0 += ap[6] * bp[96];
      s0_1 += ap[6] * bp[97];
      s1_0 += ap[22] * bp[96];
      s1_1 += ap[22] * bp[97];
      s0_0 += ap[7] * bp[112];
      s0_1 += ap[7] * bp[113];
      s1_0 += ap[23] * bp[112];
      s1_1 += ap[23] * bp[113];
      s0_0 += ap[8] * bp[128];
      s0_1 += ap[8] * bp[129];
      s1_0 += ap[24] * bp[128];
      s1_1 += ap[24] * bp[129];
      s0_0 += ap[9] * bp[144];
      s0_1 += ap[9] * bp[145];
      s1_0 += ap[25] * bp[144];
      s1_1 += ap[25] * bp[145];
      s0_0 += ap[10] * bp[160];
      s0_1 += ap[10] * bp[161];
      s1_0 += ap[26] * bp[160];
      s1_1 += ap[26] * bp[161];
      s0_0 += ap[11] * bp[176];
      s0_1 += ap[11] * bp[177];
      s1_0 += ap[27] * bp[176];
      s1_1 += ap[27] * bp[177];
      s0_0 += ap[12] * bp[192];
      s0_1 += ap[12] * bp[193];
      s1_0 += ap[28] * bp[192];
      s1_1 += ap[28] * bp[193];
      s0_0 += ap[13] * bp[208];
      s0_1 += ap[13] * bp[209];
      s1_0 += ap[29] * bp[208];
      s1_1 += ap[29] * bp[209];
      s0_0 += ap[14] * bp[224];
      s0_1 += ap[14] * bp[225];
      s1_0 += ap[30] * bp[224];
      s1_1 += ap[30] * bp[225];
      s0_0 += ap[15] * bp[240];
      s0_1 += ap[15] * bp[241];
      s1_0 += ap[31] * bp[240];
      s1_1 += ap[31] * bp[241];
      rp[0] = s0_0;
      rp[1] = s0_1;
      rp[16] = s1_0;
      rp[17] = s1_1;
    }
  }
}


/* compute R = AB, where R,A,B are BLOCK_EDGE x BLOCK_EDGE matricies
*/
static void multiply_block(block * A, block * B, block * R)
{
  int i, j;

  for (j = 0; j < 16; j += 2) {	/* 2 columns at a time */
    double *bp = &((double *) B)[j];
    for (i = 0; i < 16; i += 2) {		/* 2 rows at a time */
      double *ap = &((double *) A)[i * 16];
      double *rp = &((double *) R)[j + i * 16];
      register double s0_0, s0_1;
      register double s1_0, s1_1;
      s0_0 = ap[0] * bp[0];
      s0_1 = ap[0] * bp[1];
      s1_0 = ap[16] * bp[0];
      s1_1 = ap[16] * bp[1];
      s0_0 += ap[1] * bp[16];
      s0_1 += ap[1] * bp[17];
      s1_0 += ap[17] * bp[16];
      s1_1 += ap[17] * bp[17];
      s0_0 += ap[2] * bp[32];
      s0_1 += ap[2] * bp[33];
      s1_0 += ap[18] * bp[32];
      s1_1 += ap[18] * bp[33];
      s0_0 += ap[3] * bp[48];
      s0_1 += ap[3] * bp[49];
      s1_0 += ap[19] * bp[48];
      s1_1 += ap[19] * bp[49];
      s0_0 += ap[4] * bp[64];
      s0_1 += ap[4] * bp[65];
      s1_0 += ap[20] * bp[64];
      s1_1 += ap[20] * bp[65];
      s0_0 += ap[5] * bp[80];
      s0_1 += ap[5] * bp[81];
      s1_0 += ap[21] * bp[80];
      s1_1 += ap[21] * bp[81];
      s0_0 += ap[6] * bp[96];
      s0_1 += ap[6] * bp[97];
      s1_0 += ap[22] * bp[96];
      s1_1 += ap[22] * bp[97];
      s0_0 += ap[7] * bp[112];
      s0_1 += ap[7] * bp[113];
      s1_0 += ap[23] * bp[112];
      s1_1 += ap[23] * bp[113];
      s0_0 += ap[8] * bp[128];
      s0_1 += ap[8] * bp[129];
      s1_0 += ap[24] * bp[128];
      s1_1 += ap[24] * bp[129];
      s0_0 += ap[9] * bp[144];
      s0_1 += ap[9] * bp[145];
      s1_0 += ap[25] * bp[144];
      s1_1 += ap[25] * bp[145];
      s0_0 += ap[10] * bp[160];
      s0_1 += ap[10] * bp[161];
      s1_0 += ap[26] * bp[160];
      s1_1 += ap[26] * bp[161];
      s0_0 += ap[11] * bp[176];
      s0_1 += ap[11] * bp[177];
      s1_0 += ap[27] * bp[176];
      s1_1 += ap[27] * bp[177];
      s0_0 += ap[12] * bp[192];
      s0_1 += ap[12] * bp[193];
      s1_0 += ap[28] * bp[192];
      s1_1 += ap[28] * bp[193];
      s0_0 += ap[13] * bp[208];
      s0_1 += ap[13] * bp[209];
      s1_0 += ap[29] * bp[208];
      s1_1 += ap[29] * bp[209];
      s0_0 += ap[14] * bp[224];
      s0_1 += ap[14] * bp[225];
      s1_0 += ap[30] * bp[224];
      s1_1 += ap[30] * bp[225];
      s0_0 += ap[15] * bp[240];
      s0_1 += ap[15] * bp[241];
      s1_0 += ap[31] * bp[240];
      s1_1 += ap[31] * bp[241];
      rp[0] = s0_0;
      rp[1] = s0_1;
      rp[16] = s1_0;
      rp[17] = s1_1;
    }
  }
}


int check_matrix(block * R, long x, long y, long o, double v)
{
  int a, b;

  if (x * y == 1) {
    /**
     * Checks if each A[i,j] of a martix A of size nb x nb blocks has
     * value v.
     */
    int i;
    for (i = 0; i < BLOCK_SIZE; i++)
      if (((double *) R)[i] != v)
        return 1;

    return 0;
  }

  if (x>y) {
    a = check_matrix(R, x / 2, y, o, v);
    b = check_matrix(R + (x / 2) * o,(x + 1) / 2, y, o, v);
  } else {
    a = check_matrix(R, x, y / 2, o, v);
    b = check_matrix(R + (y / 2), x, (y + 1) / 2, o, v);
  }

  return a + b;
}

/* Add matrix T into matrix R, where T and R are bl blocks in size
 *
 */
fibril void add_matrix(block * T, long ot, block * R, long oR, long x, long y)
{
  if (x + y == 2) {
    long i;
    for (i = 0; i < BLOCK_SIZE; i += 4) {
      ((double *) R)[i + 0] += ((double *) T)[i + 0];
      ((double *) R)[i + 1] += ((double *) T)[i + 1];
      ((double *) R)[i + 2] += ((double *) T)[i + 2];
      ((double *) R)[i + 3] += ((double *) T)[i + 3];
    }
    return;
  }

  fibril_t fr;
  fibril_init(&fr);

  if (x > y) {
    fibril_fork(&fr, add_matrix, (T, ot, R, oR, x/2, y));
    add_matrix(T+(x/2)*ot, ot, R+(x/2)*oR, oR, (x+1)/2, y);
  } else {
    fibril_fork(&fr, add_matrix, (T, ot, R, oR, x, y/2));
    add_matrix(T+(y/2), ot, R+(y/2), oR, x, (y+1)/2);
  }

  fibril_join(&fr);
}

void init_matrix(block * R, long x, long y, long o, double v)
{
  if (x + y ==2) {
    int i;
    for (i = 0; i < BLOCK_SIZE; i++)
      ((double *) R)[i] = v;
    return;
  }

  if (x > y) {
    init_matrix(R, x/2, y, o, v);
    init_matrix(R+(x/2) * o, (x+1)/2, y, o, v);
  } else {
    init_matrix(R, x, y/2, o, v);
    init_matrix(R+(y/2), x, (y+1)/2, o, v);
  }
}

fibril static void multiply_matrix(block * A, long oa, block * B, long ob,
    long x, long y, long z, block * R, long oR, int add)
{
  if (x + y + z == 3) {
    if (add)
      return mult_add_block(A, B, R);
    else
      return multiply_block(A, B, R);
  }

  fibril_t fr;
  fibril_init(&fr);

  if (x >= y && x >= z) {
    fibril_fork(&fr, multiply_matrix, (A, oa, B, ob, x/2, y, z, R, oR, add));
    multiply_matrix(A+(x/2)*oa, oa, B, ob, (x+1)/2, y, z, R+(x/2)*oR, oR, add);
    fibril_join(&fr);
  } else if (y > x && y > z) {
    fibril_fork(&fr, multiply_matrix,
        (A+(y/2), oa, B+(y/2)*ob, ob, x, (y+1)/2, z, R, oR, add));

    block * tmp = malloc(x * z * sizeof(block));
    multiply_matrix(A, oa, B, ob, x, y/2, z, tmp, z, 0);
    fibril_join(&fr);

    add_matrix(tmp, z, R, oR, x, z);
    free(tmp);
  } else {
    fibril_fork(&fr, multiply_matrix, (A, oa, B, ob, x, y, z/2, R, oR, add));
    multiply_matrix(A, oa, B+(z/2), ob, x, y, (z+1)/2, R+(z/2), oR, add);
    fibril_join(&fr);
  }
}

void init() {
  x = n / BLOCK_EDGE;
  y = n / BLOCK_EDGE;
  z = n / BLOCK_EDGE;

  A = malloc(x * y * sizeof(block));
  B = malloc(y * z * sizeof(block));
  R = malloc(x * z * sizeof(block));

  init_matrix(A, x, y, y, 1.0);
  init_matrix(B, y, z, z, 1.0);
}

void prep() {
  init_matrix(R, x, z, z, 0.0);
}

void test() {
  multiply_matrix(A, y, B, z, x, y, z, R, z, 0);
}

int verify() {
#ifndef BENCHMARK
  if (check_matrix(R, x, z, z, y * 16)) {
    printf("WRONG RESULT!\n");
    return 1;
  };
#endif

  return 0;
}
