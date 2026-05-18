#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "rectmul.hpp"

import std;

namespace {

void multiply_matrix(rectmul_block *A,
                     long oa,
                     rectmul_block *B,
                     long ob,
                     long x,
                     long y,
                     long z,
                     rectmul_block *R,
                     long oR,
                     int add) {
  if (x + y + z == 3) {
    if (add != 0) {
      rectmul_mult_add_block(*A, *B, *R);
    } else {
      rectmul_multiply_block(*A, *B, *R);
    }
    return;
  }

  if (x >= y && x >= z) {
    multiply_matrix(A, oa, B, ob, x / 2, y, z, R, oR, add);
    multiply_matrix(A + (x / 2) * oa, oa, B, ob, (x + 1) / 2, y, z, R + (x / 2) * oR, oR, add);
  } else if (y > x && y > z) {
    multiply_matrix(A + (y / 2), oa, B + (y / 2) * ob, ob, x, (y + 1) / 2, z, R, oR, add);

    std::vector<rectmul_block> tmp(static_cast<std::size_t>(x * z));
    multiply_matrix(A, oa, B, ob, x, y / 2, z, tmp.data(), z, 0);
    rectmul_add_matrix(tmp.data(), z, R, oR, x, z);
  } else {
    multiply_matrix(A, oa, B, ob, x, y, z / 2, R, oR, add);
    multiply_matrix(A, oa, B + (z / 2), ob, x, y, (z + 1) / 2, R + (z / 2), oR, add);
  }
}

template <typename = void>
void rectmul_serial(benchmark::State &state) {
  run_rectmul(state, multiply_matrix);
}

} // namespace

BENCH_ALL(rectmul_serial, serial, rectmul, rectmul)
