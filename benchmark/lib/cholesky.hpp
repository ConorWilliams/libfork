#pragma once

#include <algorithm>
#include <array>

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <array>
  #include <bit>
  #include <cmath>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <memory>
#else
import std;
#endif

inline constexpr unsigned cholesky_test = 64;
inline constexpr unsigned cholesky_base = 1024;

inline constexpr unsigned cholesky_final_cutoff = 4;

struct cholesky_node {
  std::array<std::unique_ptr<cholesky_node>, 4> child{};
  std::array<double, cholesky_final_cutoff * cholesky_final_cutoff> block{};
};

using cholesky_matrix = std::unique_ptr<cholesky_node>;

enum cholesky_quadrant : unsigned {
  cholesky_00 = 0,
  cholesky_01 = 1,
  cholesky_10 = 2,
  cholesky_11 = 3,
};

inline constexpr std::array<unsigned, 4> cholesky_transpose_quadrant = {
    cholesky_00,
    cholesky_10,
    cholesky_01,
    cholesky_11,
};

struct cholesky_problem {
  cholesky_matrix initial;
  cholesky_matrix matrix;
  unsigned n;
  unsigned size;
  unsigned nonzeros;
};

struct cholesky_output {
  cholesky_node const *matrix;
  cholesky_node const *initial;
  unsigned size;
};

inline auto cholesky_copy(cholesky_node const *node) -> cholesky_matrix {
  if (node == nullptr) {
    return nullptr;
  }

  auto result = std::make_unique<cholesky_node>();
  result->block = node->block;
  for (unsigned i = 0; i < result->child.size(); ++i) {
    result->child[i] = cholesky_copy(node->child[i].get());
  }
  return result;
}

inline auto cholesky_get(cholesky_node const *node, unsigned size, unsigned row, unsigned col) -> double {
  if (node == nullptr) {
    return 0.0;
  }

  if (size == cholesky_final_cutoff) {
    return node->block[static_cast<std::size_t>(row) * cholesky_final_cutoff + col];
  }

  unsigned half = size / 2;
  if (row < half) {
    return col < half ? cholesky_get(node->child[cholesky_00].get(), half, row, col)
                      : cholesky_get(node->child[cholesky_01].get(), half, row, col - half);
  }
  return col < half ? cholesky_get(node->child[cholesky_10].get(), half, row - half, col)
                    : cholesky_get(node->child[cholesky_11].get(), half, row - half, col - half);
}

inline void cholesky_set(cholesky_matrix &node, unsigned size, unsigned row, unsigned col, double value) {
  if (node == nullptr) {
    node = std::make_unique<cholesky_node>();
  }

  if (size == cholesky_final_cutoff) {
    node->block[static_cast<std::size_t>(row) * cholesky_final_cutoff + col] = value;
    return;
  }

  unsigned half = size / 2;
  if (row < half) {
    cholesky_set(col < half ? node->child[cholesky_00] : node->child[cholesky_01],
                 half,
                 row,
                 col < half ? col : col - half,
                 value);
  } else {
    cholesky_set(col < half ? node->child[cholesky_10] : node->child[cholesky_11],
                 half,
                 row - half,
                 col < half ? col : col - half,
                 value);
  }
}

inline void cholesky_block_schur_full(cholesky_node &b, cholesky_node const &a, cholesky_node const &c) {
  for (unsigned i = 0; i < cholesky_final_cutoff; ++i) {
    for (unsigned j = 0; j < cholesky_final_cutoff; ++j) {
      for (unsigned k = 0; k < cholesky_final_cutoff; ++k) {
        b.block[i * cholesky_final_cutoff + j] -=
            a.block[i * cholesky_final_cutoff + k] * c.block[j * cholesky_final_cutoff + k];
      }
    }
  }
}

inline void cholesky_block_schur_half(cholesky_node &b, cholesky_node const &a, cholesky_node const &c) {
  for (unsigned i = 0; i < cholesky_final_cutoff; ++i) {
    for (unsigned j = 0; j <= i; ++j) {
      for (unsigned k = 0; k < cholesky_final_cutoff; ++k) {
        b.block[i * cholesky_final_cutoff + j] -=
            a.block[i * cholesky_final_cutoff + k] * c.block[j * cholesky_final_cutoff + k];
      }
    }
  }
}

inline void cholesky_block_backsub(cholesky_node &b, cholesky_node const &u) {
  for (unsigned i = 0; i < cholesky_final_cutoff; ++i) {
    for (unsigned j = 0; j < cholesky_final_cutoff; ++j) {
      for (unsigned k = 0; k < i; ++k) {
        b.block[j * cholesky_final_cutoff + i] -=
            u.block[i * cholesky_final_cutoff + k] * b.block[j * cholesky_final_cutoff + k];
      }
      b.block[j * cholesky_final_cutoff + i] /= u.block[i * cholesky_final_cutoff + i];
    }
  }
}

inline void cholesky_block_factor(cholesky_node &node) {
  for (unsigned k = 0; k < cholesky_final_cutoff; ++k) {
    double diag = std::sqrt(node.block[k * cholesky_final_cutoff + k]);
    node.block[k * cholesky_final_cutoff + k] = diag;
    for (unsigned i = k + 1; i < cholesky_final_cutoff; ++i) {
      node.block[i * cholesky_final_cutoff + k] /= diag;
    }
    for (unsigned j = k + 1; j < cholesky_final_cutoff; ++j) {
      for (unsigned i = j; i < cholesky_final_cutoff; ++i) {
        node.block[i * cholesky_final_cutoff + j] -=
            node.block[i * cholesky_final_cutoff + k] * node.block[j * cholesky_final_cutoff + k];
      }
    }
  }
}

inline void cholesky_mul_and_subT(
    unsigned size, bool lower, cholesky_node const *a, cholesky_node const *b, cholesky_matrix &r) {
  if (a == nullptr || b == nullptr) {
    return;
  }

  if (r == nullptr) {
    r = std::make_unique<cholesky_node>();
  }

  if (size == cholesky_final_cutoff) {
    lower ? cholesky_block_schur_half(*r, *a, *b) : cholesky_block_schur_full(*r, *a, *b);
    return;
  }

  unsigned half = size / 2;
  cholesky_mul_and_subT(half,
                        lower,
                        a->child[cholesky_00].get(),
                        b->child[cholesky_transpose_quadrant[cholesky_00]].get(),
                        r->child[cholesky_00]);
  if (!lower) {
    cholesky_mul_and_subT(half,
                          false,
                          a->child[cholesky_00].get(),
                          b->child[cholesky_transpose_quadrant[cholesky_01]].get(),
                          r->child[cholesky_01]);
  }
  cholesky_mul_and_subT(half,
                        false,
                        a->child[cholesky_10].get(),
                        b->child[cholesky_transpose_quadrant[cholesky_00]].get(),
                        r->child[cholesky_10]);
  cholesky_mul_and_subT(half,
                        lower,
                        a->child[cholesky_10].get(),
                        b->child[cholesky_transpose_quadrant[cholesky_01]].get(),
                        r->child[cholesky_11]);

  cholesky_mul_and_subT(half,
                        lower,
                        a->child[cholesky_01].get(),
                        b->child[cholesky_transpose_quadrant[cholesky_10]].get(),
                        r->child[cholesky_00]);
  if (!lower) {
    cholesky_mul_and_subT(half,
                          false,
                          a->child[cholesky_01].get(),
                          b->child[cholesky_transpose_quadrant[cholesky_11]].get(),
                          r->child[cholesky_01]);
  }
  cholesky_mul_and_subT(half,
                        false,
                        a->child[cholesky_11].get(),
                        b->child[cholesky_transpose_quadrant[cholesky_10]].get(),
                        r->child[cholesky_10]);
  cholesky_mul_and_subT(half,
                        lower,
                        a->child[cholesky_11].get(),
                        b->child[cholesky_transpose_quadrant[cholesky_11]].get(),
                        r->child[cholesky_11]);
}

inline void cholesky_backsub(unsigned size, cholesky_matrix &a, cholesky_node const *l) {
  if (a == nullptr || l == nullptr) {
    return;
  }

  if (size == cholesky_final_cutoff) {
    cholesky_block_backsub(*a, *l);
    return;
  }

  unsigned half = size / 2;
  cholesky_backsub(half, a->child[cholesky_00], l->child[cholesky_00].get());
  cholesky_backsub(half, a->child[cholesky_10], l->child[cholesky_00].get());
  cholesky_mul_and_subT(
      half, false, a->child[cholesky_00].get(), l->child[cholesky_10].get(), a->child[cholesky_01]);
  cholesky_mul_and_subT(
      half, false, a->child[cholesky_10].get(), l->child[cholesky_10].get(), a->child[cholesky_11]);
  cholesky_backsub(half, a->child[cholesky_01], l->child[cholesky_11].get());
  cholesky_backsub(half, a->child[cholesky_11], l->child[cholesky_11].get());
}

inline void cholesky_factor(unsigned size, cholesky_matrix &a) {
  if (a == nullptr) {
    return;
  }

  if (size == cholesky_final_cutoff) {
    cholesky_block_factor(*a);
    return;
  }

  unsigned half = size / 2;
  if (a->child[cholesky_10] == nullptr) {
    cholesky_factor(half, a->child[cholesky_00]);
    cholesky_factor(half, a->child[cholesky_11]);
  } else {
    cholesky_factor(half, a->child[cholesky_00]);
    cholesky_backsub(half, a->child[cholesky_10], a->child[cholesky_00].get());
    cholesky_mul_and_subT(
        half, true, a->child[cholesky_10].get(), a->child[cholesky_10].get(), a->child[cholesky_11]);
    cholesky_factor(half, a->child[cholesky_11]);
  }
}

inline auto cholesky_mag(cholesky_node const *node, unsigned size) -> double {
  if (node == nullptr) {
    return 0.0;
  }

  double result = 0.0;
  if (size == cholesky_final_cutoff) {
    for (double value : node->block) {
      result += value * value;
    }
    return result;
  }

  unsigned half = size / 2;
  for (auto const &child : node->child) {
    result += cholesky_mag(child.get(), half);
  }
  return result;
}

inline auto cholesky_make(unsigned n) -> cholesky_problem {
  unsigned size = std::bit_ceil(std::max(n, cholesky_final_cutoff));
  unsigned nonzeros = std::min(10U * n, n * (n + 1U) / 2U);

  cholesky_problem problem{.initial = nullptr, .matrix = nullptr, .n = n, .size = size, .nonzeros = nonzeros};

  for (unsigned i = 0; i < n; ++i) {
    cholesky_set(problem.initial, size, i, i, 1.0);
  }

  std::uint64_t rng = 1;
  for (unsigned i = n; i < nonzeros; ++i) {
    unsigned row = 0;
    unsigned col = 0;
    do {
      rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
      row = static_cast<unsigned>((rng >> 32U) % n);
      rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
      col = static_cast<unsigned>((rng >> 32U) % n);
    } while (row <= col || cholesky_get(problem.initial.get(), size, row, col) != 0.0);

    cholesky_set(problem.initial, size, row, col, 0.1);
  }

  for (unsigned i = n; i < size; ++i) {
    cholesky_set(problem.initial, size, i, i, 1.0);
  }

  return problem;
}

inline auto cholesky_is_close(cholesky_output out, double tolerance) -> bool {
  auto residual = cholesky_copy(out.initial);
  cholesky_mul_and_subT(out.size, true, out.matrix, out.matrix, residual);
  return cholesky_mag(residual.get(), out.size) <= tolerance;
}

template <typename Fn>
void run_cholesky(benchmark::State &state, std::int64_t threads, Fn fn) {
  auto n = static_cast<unsigned>(state.range(0));
  auto problem = cholesky_make(n);

  state.counters["n"] = n;
  state.counters["nonzeros"] = problem.nonzeros;
  state.counters["cutoff"] = cholesky_final_cutoff;

  lf_bench::bench(state, threads, 1e-5, cholesky_is_close, [&]() -> cholesky_output {
    state.PauseTiming();
    problem.matrix = cholesky_copy(problem.initial.get());
    state.ResumeTiming();

    std::invoke(fn, problem.matrix, problem.size);

    return {.matrix = problem.matrix.get(), .initial = problem.initial.get(), .size = problem.size};
  });
}

template <typename Fn>
void run_cholesky(benchmark::State &state, Fn fn) {
  run_cholesky(state, lf_bench::no_threads, fn);
}
