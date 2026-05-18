/*
 * Copyright (c) 1996 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to use, copy, modify, and distribute the Software without
 * restriction, provided the Software, including any modified copies made
 * under this license, is not distributed for a fee, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE MASSACHUSETTS INSTITUTE OF TECHNOLOGY BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the Massachusetts
 * Institute of Technology shall not be used in advertising or otherwise
 * to promote the sale, use or other dealings in this Software without
 * prior written authorization from the Massachusetts Institute of
 * Technology.
 *
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "test.h"

#define SizeAtWhichDivideAndConquerIsMoreEfficient 128
#define SizeAtWhichNaiveAlgorithmIsMoreEfficient 16
#define CacheBlockSizeInBytes 64

/* The real numbers we are using --- either double or float */
typedef double REAL;
typedef unsigned long PTR;

/* maximum tolerable relative error (for the checking routine) */
#define EPSILON (1.0E-6)

/*
 * Matrices are stored in row-major order; A is a pointer to
 * the first element of the matrix, and an is the number of elements
 * between two rows. This macro produces the element A[i,j]
 * given A, an, i and j
 */
#define ELEM(A, an, i, j) (A[(i) * (an) + (j)])

#ifndef BENCHMARK
int n = 512;
#else
int n = 4096;
#endif

static REAL * A, * B, * C;

#define __ALLOCATOR
#define __PARALLEL_LOOPS1
#define __PARALLEL_LOOPS2

#ifdef __ALLOCATOR
static char* memory;
static size_t memory_size;
static size_t next_alloc;

#include <assert.h>
#include <sys/mman.h>

__attribute__((always_inline)) static inline
void *alloc(size_t size) {
  size_t s = (size + CacheBlockSizeInBytes - 1) & ~(CacheBlockSizeInBytes - 1);
  size_t m = __atomic_fetch_add(&next_alloc, s, __ATOMIC_ACQ_REL);
  assert((m + s) < memory_size);
  return (void*) &(memory[m]);
}
#endif

/*
 * Naive sequential algorithm, for comparison purposes
 */
void matrixmul(int n, REAL * A, int an, REAL * B, int bn, REAL * C, int cn)
{
  int i, j, k;
  REAL s;

  for (i = 0; i < n; ++i)
    for (j = 0; j < n; ++j) {
      s = 0.0;
      for (k = 0; k < n; ++k)
        s += ELEM(A, an, i, k) * ELEM(B, bn, k, j);

      ELEM(C, cn, i, j) = s;
    }
}

/*****************************************************************************
 **
 ** FastNaiveMatrixMultiply
 **
 ** For small to medium sized matrices A, B, and C of size
 ** MatrixSize * MatrixSize this function performs the operation
 ** C = A x B efficiently.
 **
 ** Note MatrixSize must be divisible by 8.
 **
 ** INPUT:
 **    C = (*C WRITE) Address of top left element of matrix C.
 **    A = (*A IS READ ONLY) Address of top left element of matrix A.
 **    B = (*B IS READ ONLY) Address of top left element of matrix B.
 **    MatrixSize = Size of matrices (for n*n matrix, MatrixSize = n)
 **    RowWidthA = Number of elements in memory between A[x,y] and A[x,y+1]
 **    RowWidthB = Number of elements in memory between B[x,y] and B[x,y+1]
 **    RowWidthC = Number of elements in memory between C[x,y] and C[x,y+1]
 **
 ** OUTPUT:
 **    C = (*C WRITE) Matrix C contains A x B. (Initial value of *C undefined.)
 **
 *****************************************************************************/
static void FastNaiveMatrixMultiply(
    REAL * C, REAL * A, REAL * B, unsigned MatrixSize,
    unsigned RowWidthC, unsigned RowWidthA, unsigned RowWidthB)
{
  /* Assumes size of real is 8 bytes */
  PTR RowWidthBInBytes = RowWidthB  << 3;
  PTR RowWidthAInBytes = RowWidthA << 3;
  PTR MatrixWidthInBytes = MatrixSize << 3;
  PTR RowIncrementC = ( RowWidthC - MatrixSize) << 3;
  unsigned Horizontal, Vertical;

  REAL *ARowStart = A;
  for (Vertical = 0; Vertical < MatrixSize; Vertical++) {
    for (Horizontal = 0; Horizontal < MatrixSize; Horizontal += 8) {
      REAL *BColumnStart = B + Horizontal;
      REAL FirstARowValue = *ARowStart++;

      REAL Sum0 = FirstARowValue * (*BColumnStart);
      REAL Sum1 = FirstARowValue * (*(BColumnStart+1));
      REAL Sum2 = FirstARowValue * (*(BColumnStart+2));
      REAL Sum3 = FirstARowValue * (*(BColumnStart+3));
      REAL Sum4 = FirstARowValue * (*(BColumnStart+4));
      REAL Sum5 = FirstARowValue * (*(BColumnStart+5));
      REAL Sum6 = FirstARowValue * (*(BColumnStart+6));
      REAL Sum7 = FirstARowValue * (*(BColumnStart+7));

      unsigned Products;
      for (Products = 1; Products < MatrixSize; Products++) {
        REAL ARowValue = *ARowStart++;
        BColumnStart = (REAL*) (((PTR) BColumnStart) + RowWidthBInBytes);

        Sum0 += ARowValue * (*BColumnStart);
        Sum1 += ARowValue * (*(BColumnStart+1));
        Sum2 += ARowValue * (*(BColumnStart+2));
        Sum3 += ARowValue * (*(BColumnStart+3));
        Sum4 += ARowValue * (*(BColumnStart+4));
        Sum5 += ARowValue * (*(BColumnStart+5));
        Sum6 += ARowValue * (*(BColumnStart+6));
        Sum7 += ARowValue * (*(BColumnStart+7));
      }
      ARowStart = (REAL*) ( ((PTR) ARowStart) - MatrixWidthInBytes);

      *(C) = Sum0;
      *(C+1) = Sum1;
      *(C+2) = Sum2;
      *(C+3) = Sum3;
      *(C+4) = Sum4;
      *(C+5) = Sum5;
      *(C+6) = Sum6;
      *(C+7) = Sum7;
      C+=8;
    }

    ARowStart = (REAL*) ( ((PTR) ARowStart) + RowWidthAInBytes );
    C = (REAL*) ( ((PTR) C) + RowIncrementC );
  }
}

/*****************************************************************************
 **
 ** FastAdditiveNaiveMatrixMultiply
 **
 ** For small to medium sized matrices A, B, and C of size
 ** MatrixSize * MatrixSize this function performs the operation
 ** C += A x B efficiently.
 **
 ** Note MatrixSize must be divisible by 8.
 **
 ** INPUT:
 **    C = (*C READ/WRITE) Address of top left element of matrix C.
 **    A = (*A IS READ ONLY) Address of top left element of matrix A.
 **    B = (*B IS READ ONLY) Address of top left element of matrix B.
 **    MatrixSize = Size of matrices (for n*n matrix, MatrixSize = n)
 **    RowWidthA = Number of elements in memory between A[x,y] and A[x,y+1]
 **    RowWidthB = Number of elements in memory between B[x,y] and B[x,y+1]
 **    RowWidthC = Number of elements in memory between C[x,y] and C[x,y+1]
 **
 ** OUTPUT:
 **    C = (*C READ/WRITE) Matrix C contains C + A x B.
 **
 *****************************************************************************/
static void FastAdditiveNaiveMatrixMultiply(
    REAL * C, REAL * A, REAL * B, unsigned MatrixSize,
    unsigned RowWidthC, unsigned RowWidthA, unsigned RowWidthB)
{
  /* Assumes size of real is 8 bytes */
  PTR RowWidthBInBytes = RowWidthB  << 3;
  PTR RowWidthAInBytes = RowWidthA << 3;
  PTR MatrixWidthInBytes = MatrixSize << 3;
  PTR RowIncrementC = ( RowWidthC - MatrixSize) << 3;
  unsigned Horizontal, Vertical;

  REAL *ARowStart = A;
  for (Vertical = 0; Vertical < MatrixSize; Vertical++) {
    for (Horizontal = 0; Horizontal < MatrixSize; Horizontal += 8) {
      REAL *BColumnStart = B + Horizontal;

      REAL Sum0 = *C;
      REAL Sum1 = *(C+1);
      REAL Sum2 = *(C+2);
      REAL Sum3 = *(C+3);
      REAL Sum4 = *(C+4);
      REAL Sum5 = *(C+5);
      REAL Sum6 = *(C+6);
      REAL Sum7 = *(C+7);

      unsigned Products;
      for (Products = 0; Products < MatrixSize; Products++) {
        REAL ARowValue = *ARowStart++;

        Sum0 += ARowValue * (*BColumnStart);
        Sum1 += ARowValue * (*(BColumnStart+1));
        Sum2 += ARowValue * (*(BColumnStart+2));
        Sum3 += ARowValue * (*(BColumnStart+3));
        Sum4 += ARowValue * (*(BColumnStart+4));
        Sum5 += ARowValue * (*(BColumnStart+5));
        Sum6 += ARowValue * (*(BColumnStart+6));
        Sum7 += ARowValue * (*(BColumnStart+7));

        BColumnStart = (REAL*) (((PTR) BColumnStart) + RowWidthBInBytes);

      }
      ARowStart = (REAL*) ( ((PTR) ARowStart) - MatrixWidthInBytes);

      *(C) = Sum0;
      *(C+1) = Sum1;
      *(C+2) = Sum2;
      *(C+3) = Sum3;
      *(C+4) = Sum4;
      *(C+5) = Sum5;
      *(C+6) = Sum6;
      *(C+7) = Sum7;
      C+=8;
    }

    ARowStart = (REAL*) ( ((PTR) ARowStart) + RowWidthAInBytes );
    C = (REAL*) ( ((PTR) C) + RowIncrementC );
  }
}


/*****************************************************************************
 **
 ** MultiplyByDivideAndConquer
 **
 ** For medium to medium-large (would you like fries with that) sized
 ** matrices A, B, and C of size MatrixSize * MatrixSize this function
 ** efficiently performs the operation
 **    C  = A x B (if AdditiveMode == 0)
 **    C += A x B (if AdditiveMode != 0)
 **
 ** Note MatrixSize must be divisible by 16.
 **
 ** INPUT:
 **    C = (*C READ/WRITE) Address of top left element of matrix C.
 **    A = (*A IS READ ONLY) Address of top left element of matrix A.
 **    B = (*B IS READ ONLY) Address of top left element of matrix B.
 **    MatrixSize = Size of matrices (for n*n matrix, MatrixSize = n)
 **    RowWidthA = Number of elements in memory between A[x,y] and A[x,y+1]
 **    RowWidthB = Number of elements in memory between B[x,y] and B[x,y+1]
 **    RowWidthC = Number of elements in memory between C[x,y] and C[x,y+1]
 **    AdditiveMode = 0 if we want C = A x B, otherwise we'll do C += A x B
 **
 ** OUTPUT:
 **    C (+)= A x B. (+ if AdditiveMode != 0)
 **
 *****************************************************************************/
void MultiplyByDivideAndConquer(
    REAL * C, REAL * A, REAL * B, unsigned MatrixSize,
    unsigned RowWidthC, unsigned RowWidthA, unsigned RowWidthB,
    int AdditiveMode)
{
#define A00 A
#define B00 B
#define C00 C

  REAL  *A01, *A10, *A11, *B01, *B10, *B11, *C01, *C10, *C11;
  unsigned QuadrantSize = MatrixSize >> 1;

  /* partition the matrix */
  A01 = A00 + QuadrantSize;
  A10 = A00 + RowWidthA * QuadrantSize;
  A11 = A10 + QuadrantSize;

  B01 = B00 + QuadrantSize;
  B10 = B00 + RowWidthB * QuadrantSize;
  B11 = B10 + QuadrantSize;

  C01 = C00 + QuadrantSize;
  C10 = C00 + RowWidthC * QuadrantSize;
  C11 = C10 + QuadrantSize;

  if (QuadrantSize > SizeAtWhichNaiveAlgorithmIsMoreEfficient) {
    MultiplyByDivideAndConquer(C00, A00, B00, QuadrantSize,
        RowWidthC, RowWidthA, RowWidthB, AdditiveMode);
    MultiplyByDivideAndConquer(C01, A00, B01, QuadrantSize,
        RowWidthC, RowWidthA, RowWidthB, AdditiveMode);
    MultiplyByDivideAndConquer(C11, A10, B01, QuadrantSize,
        RowWidthC, RowWidthA, RowWidthB, AdditiveMode);
    MultiplyByDivideAndConquer(C10, A10, B00, QuadrantSize,
        RowWidthC, RowWidthA, RowWidthB, AdditiveMode);
    MultiplyByDivideAndConquer(C00, A01, B10, QuadrantSize,
        RowWidthC, RowWidthA, RowWidthB, 1);
    MultiplyByDivideAndConquer(C01, A01, B11, QuadrantSize,
        RowWidthC, RowWidthA, RowWidthB, 1);
    MultiplyByDivideAndConquer(C11, A11, B11, QuadrantSize,
        RowWidthC, RowWidthA, RowWidthB, 1);
    MultiplyByDivideAndConquer(C10, A11, B10, QuadrantSize,
        RowWidthC, RowWidthA, RowWidthB, 1);
  } else {
    if (AdditiveMode) {
      FastAdditiveNaiveMatrixMultiply(C00, A00, B00, QuadrantSize,
          RowWidthC, RowWidthA, RowWidthB);
      FastAdditiveNaiveMatrixMultiply(C01, A00, B01, QuadrantSize,
          RowWidthC, RowWidthA, RowWidthB);
      FastAdditiveNaiveMatrixMultiply(C11, A10, B01, QuadrantSize,
          RowWidthC, RowWidthA, RowWidthB);
      FastAdditiveNaiveMatrixMultiply(C10, A10, B00, QuadrantSize,
          RowWidthC, RowWidthA, RowWidthB);
    } else {
      FastNaiveMatrixMultiply(C00, A00, B00, QuadrantSize,
          RowWidthC, RowWidthA, RowWidthB);

      FastNaiveMatrixMultiply(C01, A00, B01, QuadrantSize,
          RowWidthC, RowWidthA, RowWidthB);

      FastNaiveMatrixMultiply(C11, A10, B01, QuadrantSize,
          RowWidthC, RowWidthA, RowWidthB);

      FastNaiveMatrixMultiply(C10, A10, B00, QuadrantSize,
          RowWidthC, RowWidthA, RowWidthB);
    }

    FastAdditiveNaiveMatrixMultiply(C00, A01, B10, QuadrantSize,
        RowWidthC, RowWidthA, RowWidthB);
    FastAdditiveNaiveMatrixMultiply(C01, A01, B11, QuadrantSize,
        RowWidthC, RowWidthA, RowWidthB);
    FastAdditiveNaiveMatrixMultiply(C11, A11, B11, QuadrantSize,
        RowWidthC, RowWidthA, RowWidthB);
    FastAdditiveNaiveMatrixMultiply(C10, A11, B10, QuadrantSize,
        RowWidthC, RowWidthA, RowWidthB);
  }

  return;
}


#ifdef __PARALLEL_LOOPS1
struct parallel_loop_param1 {
  REAL *A11;
  REAL *A12;
  REAL *A21;
  REAL *A22;
  REAL *B11;
  REAL *B12;
  REAL *B21;
  REAL *B22;
  REAL *S1;
  REAL *S2;
  REAL *S3;
  REAL *S4;
  REAL *S5;
  REAL *S6;
  REAL *S7;
  REAL *S8;
  REAL *M2;
  REAL *M5;
  REAL *T1sMULT;
  unsigned QuadrantSize;
  PTR RowIncrementA;
  PTR RowIncrementB;
};


fibril static void parallel_loop1(
    struct parallel_loop_param1 *param,
    unsigned RowL,
    unsigned RowU)
{
  unsigned Column;
  REAL *A11, *A12, *A21, *A22, *B11, *B12, *B21, *B22, *S1, *S2,
       *S3, *S4, *S5, *S6, *S7, *S8, *M2, *M5, *T1sMULT;
  PTR TempMatrixOffset, MatrixOffsetA, MatrixOffsetB, RowIncrementA, RowIncrementB;
  unsigned QuadrantSize;

  if (RowU - RowL > 1) {
    unsigned RowM = (RowL + RowU) / 2;
    fibril_t fr;
    fibril_init(&fr);

    fibril_fork(&fr, parallel_loop1,
        (param, RowL, RowM));
    parallel_loop1(param, RowM, RowU);

    fibril_join(&fr);

    return;
  }

  A11 = param->A11;
  A12 = param->A12;
  A21 = param->A21;
  A22 = param->A22;
  B11 = param->B11;
  B12 = param->B12;
  B21 = param->B21;
  B22 = param->B22;
  S1 = param->S1;
  S2 = param->S2;
  S3 = param->S3;
  S4 = param->S4;
  S5 = param->S5;
  S6 = param->S6;
  S7 = param->S7;
  S8 = param->S8;
  M2 = param->M2;
  M5 = param->M5;
  T1sMULT = param->T1sMULT;
  QuadrantSize = param->QuadrantSize;
  RowIncrementA = param->RowIncrementA;
  RowIncrementB = param->RowIncrementB;

  TempMatrixOffset = RowL * (sizeof(REAL) * QuadrantSize);
  MatrixOffsetA = RowL * ((sizeof(REAL) * QuadrantSize) + RowIncrementA);
  MatrixOffsetB = RowL * ((sizeof(REAL) * QuadrantSize) + RowIncrementB);

  for (Column = 0; Column < QuadrantSize; Column++) {

    /***********************************************************
     ** Within this loop, the following holds for MatrixOffset:
     ** MatrixOffset = (Row * RowWidth) + Column
     ** (note: that the unit of the offset is number of reals)
     ***********************************************************/
    /* Element of Global Matrix, such as A, B, C */
#define E(Matrix)   (* (REAL*) ( ((PTR) (Matrix)) + TempMatrixOffset ) )
#define EA(Matrix)  (* (REAL*) ( ((PTR) (Matrix)) + MatrixOffsetA ) )
#define EB(Matrix)  (* (REAL*) ( ((PTR) (Matrix)) + MatrixOffsetB ) )

    /* FIXME - may pay to expand these out - got higher speed-ups below */
    /* S4 = A12 - ( S2 = ( S1 = A21 + A22 ) - A11 ) */
    E(S4) = EA(A12) - ( E(S2) = ( E(S1) = EA(A21) + EA(A22) ) - EA(A11) );

    /* S8 = (S6 = B22 - ( S5 = B12 - B11 ) ) - B21 */
    E(S8) = ( E(S6) = EB(B22) - ( E(S5) = EB(B12) - EB(B11) ) ) - EB(B21);

    /* S3 = A11 - A21 */
    E(S3) = EA(A11) - EA(A21);

    /* S7 = B22 - B12 */
    E(S7) = EB(B22) - EB(B12);

    TempMatrixOffset += sizeof(REAL);
    MatrixOffsetA += sizeof(REAL);
    MatrixOffsetB += sizeof(REAL);
  } /* end row loop*/
#undef E
#undef EA
#undef EB
}
#endif


#ifdef __PARALLEL_LOOPS2
fibril static void parallel_loop2(
    REAL *C11,
    REAL *C12,
    REAL *C21,
    REAL *C22,
    REAL *M2,
    REAL *M5,
    REAL *T1sMULT,
    PTR RowIncrementC,
    unsigned QuadrantSize,
    unsigned RowL,
    unsigned RowU)
{
  unsigned Column;

  if (RowU - RowL > 1) {
    unsigned RowM = (RowL + RowU) / 2;
    fibril_t fr;
    fibril_init(&fr);

    fibril_fork(&fr, parallel_loop2,
        (C11, C12, C21, C22, M2, M5, T1sMULT, RowIncrementC, QuadrantSize, RowL, RowM));
    parallel_loop2(C11, C12, C21, C22, M2, M5, T1sMULT, RowIncrementC, QuadrantSize, RowM, RowU);

    fibril_join(&fr);

    return;
  }

  M5 += RowL * QuadrantSize;
  M2 += RowL * QuadrantSize;
  T1sMULT += RowL * QuadrantSize;
  C11 += RowL * QuadrantSize;
  C12 += RowL * QuadrantSize;
  C21 += RowL * QuadrantSize;
  C22 += RowL * QuadrantSize;
  C11 = (REAL*) ( ((PTR) C11 ) + RowL * RowIncrementC);
  C12 = (REAL*) ( ((PTR) C12 ) + RowL * RowIncrementC);
  C21 = (REAL*) ( ((PTR) C21 ) + RowL * RowIncrementC);
  C22 = (REAL*) ( ((PTR) C22 ) + RowL * RowIncrementC);

  for (Column = 0; Column < QuadrantSize; Column += 4) {
    REAL LocalM5_0 = *(M5);
    REAL LocalM5_1 = *(M5+1);
    REAL LocalM5_2 = *(M5+2);
    REAL LocalM5_3 = *(M5+3);
    REAL LocalM2_0 = *(M2);
    REAL LocalM2_1 = *(M2+1);
    REAL LocalM2_2 = *(M2+2);
    REAL LocalM2_3 = *(M2+3);
    REAL T1_0 = *(T1sMULT) + LocalM2_0;
    REAL T1_1 = *(T1sMULT+1) + LocalM2_1;
    REAL T1_2 = *(T1sMULT+2) + LocalM2_2;
    REAL T1_3 = *(T1sMULT+3) + LocalM2_3;
    REAL T2_0 = *(C22) + T1_0;
    REAL T2_1 = *(C22+1) + T1_1;
    REAL T2_2 = *(C22+2) + T1_2;
    REAL T2_3 = *(C22+3) + T1_3;
    (*(C11))   += LocalM2_0;
    (*(C11+1)) += LocalM2_1;
    (*(C11+2)) += LocalM2_2;
    (*(C11+3)) += LocalM2_3;
    (*(C12))   += LocalM5_0 + T1_0;
    (*(C12+1)) += LocalM5_1 + T1_1;
    (*(C12+2)) += LocalM5_2 + T1_2;
    (*(C12+3)) += LocalM5_3 + T1_3;
    (*(C22))   = LocalM5_0 + T2_0;
    (*(C22+1)) = LocalM5_1 + T2_1;
    (*(C22+2)) = LocalM5_2 + T2_2;
    (*(C22+3)) = LocalM5_3 + T2_3;
    (*(C21  )) = (- *(C21  )) + T2_0;
    (*(C21+1)) = (- *(C21+1)) + T2_1;
    (*(C21+2)) = (- *(C21+2)) + T2_2;
    (*(C21+3)) = (- *(C21+3)) + T2_3;
    M5 += 4;
    M2 += 4;
    T1sMULT += 4;
    C11 += 4;
    C12 += 4;
    C21 += 4;
    C22 += 4;
  }
}
#endif


/*****************************************************************************
 **
 ** OptimizedStrassenMultiply
 **
 ** For large matrices A, B, and C of size MatrixSize * MatrixSize this
 ** function performs the operation C = A x B efficiently.
 **
 ** INPUT:
 **    C = (*C WRITE) Address of top left element of matrix C.
 **    A = (*A IS READ ONLY) Address of top left element of matrix A.
 **    B = (*B IS READ ONLY) Address of top left element of matrix B.
 **    MatrixSize = Size of matrices (for n*n matrix, MatrixSize = n)
 **    RowWidthA = Number of elements in memory between A[x,y] and A[x,y+1]
 **    RowWidthB = Number of elements in memory between B[x,y] and B[x,y+1]
 **    RowWidthC = Number of elements in memory between C[x,y] and C[x,y+1]
 ** OUTPUT:
 **    C = (*C WRITE) Matrix C contains A x B. (Initial value of *C undefined.)
 **
 *****************************************************************************/
fibril static void OptimizedStrassenMultiply(
    REAL * C, REAL * A, REAL * B, unsigned MatrixSize,
    unsigned RowWidthC, unsigned RowWidthA, unsigned RowWidthB)
{
  unsigned QuadrantSize = MatrixSize >> 1; /* MatixSize / 2 */
  unsigned QuadrantSizeInBytes = sizeof(REAL) * QuadrantSize *
    QuadrantSize + CacheBlockSizeInBytes;
  unsigned Column, Row;

  /************************************************************************
   ** For each matrix A, B, and C, we'll want pointers to each quandrant
   ** in the matrix. These quandrants will be addressed as follows:
   **  --        --
   **  | A11  A12 |
   **  |          |
   **  | A21  A22 |
   **  --        --
   ************************************************************************/
  REAL /**A11, *B11, *C11,*/ *A12, *B12, *C12,
     *A21, *B21, *C21, *A22, *B22, *C22;

  REAL *S1,*S2,*S3,*S4,*S5,*S6,*S7,*S8,*M2,*M5,*T1sMULT;
#define NumberOfVariables 11

  PTR TempMatrixOffset = 0;
  PTR MatrixOffsetA = 0;
  PTR MatrixOffsetB = 0;

  char *Heap;
  void *StartHeap;

  /* Distance between the end of a matrix row and the start of the next row */
  PTR RowIncrementA = ( RowWidthA - QuadrantSize ) << 3;
  PTR RowIncrementB = ( RowWidthB - QuadrantSize ) << 3;
  PTR RowIncrementC = ( RowWidthC - QuadrantSize ) << 3;

  if (MatrixSize <= SizeAtWhichDivideAndConquerIsMoreEfficient) {
    MultiplyByDivideAndConquer(C, A, B, MatrixSize,
        RowWidthC, RowWidthA, RowWidthB, 0);
    return;
  }

  /* Initialize quandrant matrices */
#define A11 A
#define B11 B
#define C11 C
  A12 = A11 + QuadrantSize;
  B12 = B11 + QuadrantSize;
  C12 = C11 + QuadrantSize;
  A21 = A + (RowWidthA * QuadrantSize);
  B21 = B + (RowWidthB * QuadrantSize);
  C21 = C + (RowWidthC * QuadrantSize);
  A22 = A21 + QuadrantSize;
  B22 = B21 + QuadrantSize;
  C22 = C21 + QuadrantSize;

  /* Allocate Heap Space Here */
#ifdef __ALLOCATOR
  StartHeap = Heap = alloc(QuadrantSizeInBytes * NumberOfVariables);
#else
  StartHeap = Heap = malloc(QuadrantSizeInBytes * NumberOfVariables);
#endif
  /* ensure that heap is on cache boundary */
  if ( ((PTR) Heap) & (CacheBlockSizeInBytes - 1))
    Heap = (char*) ( ((PTR) Heap) + CacheBlockSizeInBytes - ( ((PTR) Heap) & (CacheBlockSizeInBytes - 1)) );

  /* Distribute the heap space over the variables */
  S1 = (REAL*) Heap; Heap += QuadrantSizeInBytes;
  S2 = (REAL*) Heap; Heap += QuadrantSizeInBytes;
  S3 = (REAL*) Heap; Heap += QuadrantSizeInBytes;
  S4 = (REAL*) Heap; Heap += QuadrantSizeInBytes;
  S5 = (REAL*) Heap; Heap += QuadrantSizeInBytes;
  S6 = (REAL*) Heap; Heap += QuadrantSizeInBytes;
  S7 = (REAL*) Heap; Heap += QuadrantSizeInBytes;
  S8 = (REAL*) Heap; Heap += QuadrantSizeInBytes;
  M2 = (REAL*) Heap; Heap += QuadrantSizeInBytes;
  M5 = (REAL*) Heap; Heap += QuadrantSizeInBytes;
  T1sMULT = (REAL*) Heap; Heap += QuadrantSizeInBytes;

  fibril_t fr;
  fibril_init(&fr);

#ifdef __PARALLEL_LOOPS1
  struct parallel_loop_param1 param =
  {A11, A12, A21, A22, B11, B12, B21, B22, S1, S2, S3, S4, S5, S6, S7, S8, M2, M5,
    T1sMULT, QuadrantSize, RowIncrementA, RowIncrementB};
  parallel_loop1(&param, 0, QuadrantSize);
#else
  /***************************************************************************
   ** Step through all columns row by row (vertically)
   ** (jumps in memory by RowWidth => bad locality)
   ** (but we want the best locality on the innermost loop)
   ***************************************************************************/
  for (Row = 0; Row < QuadrantSize; Row++) {

    /*************************************************************************
     ** Step through each row horizontally (addressing elements in each column)
     ** (jumps linearly througn memory => good locality)
     *************************************************************************/
    for (Column = 0; Column < QuadrantSize; Column++) {

      /***********************************************************
       ** Within this loop, the following holds for MatrixOffset:
       ** MatrixOffset = (Row * RowWidth) + Column
       ** (note: that the unit of the offset is number of reals)
       ***********************************************************/
      /* Element of Global Matrix, such as A, B, C */
#define E(Matrix)   (* (REAL*) ( ((PTR) Matrix) + TempMatrixOffset ) )
#define EA(Matrix)  (* (REAL*) ( ((PTR) Matrix) + MatrixOffsetA ) )
#define EB(Matrix)  (* (REAL*) ( ((PTR) Matrix) + MatrixOffsetB ) )

      /* FIXME - may pay to expand these out - got higher speed-ups below */
      /* S4 = A12 - ( S2 = ( S1 = A21 + A22 ) - A11 ) */
      E(S4) = EA(A12) - ( E(S2) = ( E(S1) = EA(A21) + EA(A22) ) - EA(A11) );

      /* S8 = (S6 = B22 - ( S5 = B12 - B11 ) ) - B21 */
      E(S8) = ( E(S6) = EB(B22) - ( E(S5) = EB(B12) - EB(B11) ) ) - EB(B21);

      /* S3 = A11 - A21 */
      E(S3) = EA(A11) - EA(A21);

      /* S7 = B22 - B12 */
      E(S7) = EB(B22) - EB(B12);

      TempMatrixOffset += sizeof(REAL);
      MatrixOffsetA += sizeof(REAL);
      MatrixOffsetB += sizeof(REAL);
    } /* end row loop*/

    MatrixOffsetA += RowIncrementA;
    MatrixOffsetB += RowIncrementB;
  } /* end column loop */
#endif

  /* M2 = A11 x B11 */
  fibril_fork(&fr, OptimizedStrassenMultiply,
      (M2, A11, B11, QuadrantSize, QuadrantSize, RowWidthA, RowWidthB));

  /* M5 = S1 * S5 */
  fibril_fork(&fr, OptimizedStrassenMultiply,
      (M5, S1, S5, QuadrantSize, QuadrantSize, QuadrantSize, QuadrantSize));

  /* Step 1 of T1 = S2 x S6 + M2 */
  fibril_fork(&fr, OptimizedStrassenMultiply,
      (T1sMULT, S2, S6, QuadrantSize, QuadrantSize, QuadrantSize, QuadrantSize));

  /* Step 1 of T2 = T1 + S3 x S7 */
  fibril_fork(&fr, OptimizedStrassenMultiply,
      (C22, S3, S7, QuadrantSize, RowWidthC, QuadrantSize, QuadrantSize));

  /* Step 1 of C11 = M2 + A12 * B21 */
  fibril_fork(&fr, OptimizedStrassenMultiply,
      (C11, A12, B21, QuadrantSize, RowWidthC, RowWidthA, RowWidthB));

  /* Step 1 of C12 = S4 x B22 + T1 + M5 */
  fibril_fork(&fr, OptimizedStrassenMultiply,
    (C12, S4, B22, QuadrantSize, RowWidthC, QuadrantSize, RowWidthB));

  /* Step 1 of C21 = T2 - A22 * S8 */
  OptimizedStrassenMultiply(C21, A22, S8, QuadrantSize, RowWidthC,
      RowWidthA, QuadrantSize);

  fibril_join(&fr);

#ifdef __PARALLEL_LOOPS2
  parallel_loop2(C11, C12, C21, C22, M2, M5, T1sMULT, RowIncrementC, QuadrantSize, 0, QuadrantSize);
#else
  for (Row = 0; Row < QuadrantSize; Row++) {
    for (Column = 0; Column < QuadrantSize; Column += 4) {
      REAL LocalM5_0 = *(M5);
      REAL LocalM5_1 = *(M5+1);
      REAL LocalM5_2 = *(M5+2);
      REAL LocalM5_3 = *(M5+3);
      REAL LocalM2_0 = *(M2);
      REAL LocalM2_1 = *(M2+1);
      REAL LocalM2_2 = *(M2+2);
      REAL LocalM2_3 = *(M2+3);
      REAL T1_0 = *(T1sMULT) + LocalM2_0;
      REAL T1_1 = *(T1sMULT+1) + LocalM2_1;
      REAL T1_2 = *(T1sMULT+2) + LocalM2_2;
      REAL T1_3 = *(T1sMULT+3) + LocalM2_3;
      REAL T2_0 = *(C22) + T1_0;
      REAL T2_1 = *(C22+1) + T1_1;
      REAL T2_2 = *(C22+2) + T1_2;
      REAL T2_3 = *(C22+3) + T1_3;
      (*(C11))   += LocalM2_0;
      (*(C11+1)) += LocalM2_1;
      (*(C11+2)) += LocalM2_2;
      (*(C11+3)) += LocalM2_3;
      (*(C12))   += LocalM5_0 + T1_0;
      (*(C12+1)) += LocalM5_1 + T1_1;
      (*(C12+2)) += LocalM5_2 + T1_2;
      (*(C12+3)) += LocalM5_3 + T1_3;
      (*(C22))   = LocalM5_0 + T2_0;
      (*(C22+1)) = LocalM5_1 + T2_1;
      (*(C22+2)) = LocalM5_2 + T2_2;
      (*(C22+3)) = LocalM5_3 + T2_3;
      (*(C21  )) = (- *(C21  )) + T2_0;
      (*(C21+1)) = (- *(C21+1)) + T2_1;
      (*(C21+2)) = (- *(C21+2)) + T2_2;
      (*(C21+3)) = (- *(C21+3)) + T2_3;
      M5 += 4;
      M2 += 4;
      T1sMULT += 4;
      C11 += 4;
      C12 += 4;
      C21 += 4;
      C22 += 4;
    }

    C11 = (REAL*) ( ((PTR) C11 ) + RowIncrementC);
    C12 = (REAL*) ( ((PTR) C12 ) + RowIncrementC);
    C21 = (REAL*) ( ((PTR) C21 ) + RowIncrementC);
    C22 = (REAL*) ( ((PTR) C22 ) + RowIncrementC);
  }
#endif

#ifndef __ALLOCATOR
  free(StartHeap);
#endif
}

static void strassen(int n, REAL * A, int an, REAL * B, int bn,
    REAL * C, int cn) {
  OptimizedStrassenMultiply(C, A, B, n, cn, bn, an);
}

/*
 * Set an n by n matrix A to random values.  The distance between
 * rows is an
 */
void init_matrix(int n, REAL *A, int an)
{
  int i, j;

  for (i = 0; i < n; ++i)
    for (j = 0; j < n; ++j)
      ELEM(A, an, i, j) = ((double) rand()) / (double) RAND_MAX;
}

/*
 * Compare two matrices.  Print an error message if they differ by
 * more than EPSILON.
 */
int compare_matrix(int n, REAL *A, int an, REAL *B, int bn)
{
  int i, j;
  REAL c;

  for (i = 0; i < n; ++i)
    for (j = 0; j < n; ++j) {
      /* compute the relative error c */
      c = ELEM(A, an, i, j) - ELEM(B, bn, i, j);
      if (c < 0.0)
        c = -c;

      c = c / ELEM(A, an, i, j);
      if (c > EPSILON) {
        return 1;
      }
    }

  return 0;
}

void init() {
  A = malloc(n * n * sizeof(REAL));
  B = malloc(n * n * sizeof(REAL));
  C = malloc(n * n * sizeof(REAL));

  init_matrix(n, A, n);
  init_matrix(n, B, n);

#ifdef __ALLOCATOR
  size_t s = 0;
  size_t ins = 1;
  for (unsigned int i = n; i > SizeAtWhichDivideAndConquerIsMoreEfficient; i >>= 1) {
    unsigned int m = i >> 1;
    s += (8*m*m + 11*CacheBlockSizeInBytes) * 11*ins;
    ins *= 7;
  }
  memory_size = (s + 4096) & ~4095;
  memory = mmap(NULL, memory_size, PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE,
      -1, 0);
  assert(memory != MAP_FAILED);
  next_alloc = 0;
#endif
}

void prep() {
#ifdef __ALLOCATOR
  next_alloc = 0;
#endif
}

void test() {
  strassen(n, A, n, B, n, C, n);
}

int verify() {
  int fail = 0;

#ifndef BENCHMARK
  REAL * E = malloc(n * n * sizeof(REAL));
  matrixmul(n, A, n, B, n, E, n);
  fail = compare_matrix(n, E, n, C, n);
  if (fail > 0) printf("WRONG RESULT!\n");
#endif

  return fail;
}
