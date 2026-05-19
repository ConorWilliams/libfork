#include <benchmark/benchmark.h>

#include "lu.hpp"
#include "macros.hpp"

import std;

namespace {

void lu_serial(lu_block *M, unsigned stride, unsigned nb);

void schur_serial(lu_block *M, lu_block *V, lu_block *W, unsigned stride, unsigned nb) {
  if (nb == 1) {
    lu_block_schur(*M, *V, *W);
    return;
  }

  unsigned h = nb / 2;
  auto *M00 = lu_block_at(M, stride, 0, 0);
  auto *M01 = lu_block_at(M, stride, 0, h);
  auto *M10 = lu_block_at(M, stride, h, 0);
  auto *M11 = lu_block_at(M, stride, h, h);
  auto *V00 = lu_block_at(V, stride, 0, 0);
  auto *V01 = lu_block_at(V, stride, 0, h);
  auto *V10 = lu_block_at(V, stride, h, 0);
  auto *V11 = lu_block_at(V, stride, h, h);
  auto *W00 = lu_block_at(W, stride, 0, 0);
  auto *W01 = lu_block_at(W, stride, 0, h);
  auto *W10 = lu_block_at(W, stride, h, 0);
  auto *W11 = lu_block_at(W, stride, h, h);

  schur_serial(M00, V00, W00, stride, h);
  schur_serial(M01, V00, W01, stride, h);
  schur_serial(M10, V10, W00, stride, h);
  schur_serial(M11, V10, W01, stride, h);

  schur_serial(M00, V01, W10, stride, h);
  schur_serial(M01, V01, W11, stride, h);
  schur_serial(M10, V11, W10, stride, h);
  schur_serial(M11, V11, W11, stride, h);
}

void lower_solve_serial(lu_block *M, lu_block *L, unsigned stride, unsigned nb) {
  if (nb == 1) {
    lu_block_lower_solve(*M, *L);
    return;
  }

  unsigned h = nb / 2;
  auto *M00 = lu_block_at(M, stride, 0, 0);
  auto *M01 = lu_block_at(M, stride, 0, h);
  auto *M10 = lu_block_at(M, stride, h, 0);
  auto *M11 = lu_block_at(M, stride, h, h);
  auto *L00 = lu_block_at(L, stride, 0, 0);
  auto *L10 = lu_block_at(L, stride, h, 0);
  auto *L11 = lu_block_at(L, stride, h, h);

  lower_solve_serial(M00, L00, stride, h);
  schur_serial(M10, L10, M00, stride, h);
  lower_solve_serial(M10, L11, stride, h);

  lower_solve_serial(M01, L00, stride, h);
  schur_serial(M11, L10, M01, stride, h);
  lower_solve_serial(M11, L11, stride, h);
}

void upper_solve_serial(lu_block *M, lu_block *U, unsigned stride, unsigned nb) {
  if (nb == 1) {
    lu_block_upper_solve(*M, *U);
    return;
  }

  unsigned h = nb / 2;
  auto *M00 = lu_block_at(M, stride, 0, 0);
  auto *M01 = lu_block_at(M, stride, 0, h);
  auto *M10 = lu_block_at(M, stride, h, 0);
  auto *M11 = lu_block_at(M, stride, h, h);
  auto *U00 = lu_block_at(U, stride, 0, 0);
  auto *U01 = lu_block_at(U, stride, 0, h);
  auto *U11 = lu_block_at(U, stride, h, h);

  upper_solve_serial(M00, U00, stride, h);
  schur_serial(M01, M00, U01, stride, h);
  upper_solve_serial(M01, U11, stride, h);

  upper_solve_serial(M10, U00, stride, h);
  schur_serial(M11, M10, U01, stride, h);
  upper_solve_serial(M11, U11, stride, h);
}

void lu_serial(lu_block *M, unsigned stride, unsigned nb) {
  if (nb == 1) {
    lu_block_lu(*M);
    return;
  }

  unsigned h = nb / 2;
  auto *M00 = lu_block_at(M, stride, 0, 0);
  auto *M01 = lu_block_at(M, stride, 0, h);
  auto *M10 = lu_block_at(M, stride, h, 0);
  auto *M11 = lu_block_at(M, stride, h, h);

  lu_serial(M00, stride, h);
  lower_solve_serial(M01, M00, stride, h);
  upper_solve_serial(M10, M00, stride, h);
  schur_serial(M11, M10, M01, stride, h);
  lu_serial(M11, stride, h);
}

template <typename = void>
void lu_serial_bench(benchmark::State &state) {
  run_lu(state, [](lu_block *M, unsigned blocks) {
    lu_serial(M, blocks, blocks);
  });
}

} // namespace

BENCH_ALL(lu_serial_bench, serial, lu, lu)
