#pragma once

#include <algorithm>
#include <array>

#include "bench.hpp"

#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <array>
  #include <cmath>
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <memory>
  #include <vector>
#else
import std;
#endif

inline constexpr std::int64_t fft_test = 12;
inline constexpr std::int64_t fft_base = 26;

inline constexpr unsigned fft_unshuffle_cutoff = 16;
inline constexpr unsigned fft_twiddle_cutoff = 128;
inline constexpr unsigned fft_base_cutoff = 32;
inline constexpr double fft_pi = 3.141592653589793238462643383279502884;
inline constexpr std::array<unsigned, 8> fft_check_indices = {0, 1, 3, 5, 17, 97, 251, 509};

struct fft_complex {
  float re;
  float im;
};

struct fft_problem {
  std::unique_ptr<fft_complex[]> input;
  std::unique_ptr<fft_complex[]> work;
  std::unique_ptr<fft_complex[]> output;
  std::unique_ptr<fft_complex[]> roots;
  std::array<fft_complex, fft_check_indices.size()> expected;
  unsigned n;
};

struct fft_output {
  fft_complex const *output;
  std::array<fft_complex, fft_check_indices.size()> const *expected;
  unsigned n;
};

constexpr auto operator+(fft_complex a, fft_complex b) -> fft_complex {
  return {.re = a.re + b.re, .im = a.im + b.im};
}

constexpr auto operator-(fft_complex a, fft_complex b) -> fft_complex {
  return {.re = a.re - b.re, .im = a.im - b.im};
}

constexpr auto operator*(fft_complex a, fft_complex b) -> fft_complex {
  return {.re = a.re * b.re - a.im * b.im, .im = a.re * b.im + a.im * b.re};
}

inline auto fft_abs(fft_complex z) -> double {
  double re = static_cast<double>(z.re);
  double im = static_cast<double>(z.im);
  return std::sqrt(re * re + im * im);
}

inline auto fft_exp(double angle) -> fft_complex {
  return {.re = static_cast<float>(std::cos(angle)), .im = static_cast<float>(std::sin(angle))};
}

inline auto fft_input_value(unsigned i) -> fft_complex {
  return {
      .re = static_cast<float>(((i + 1U) * 17U + (i >> 3U)) % 257U) / 257.0F,
      .im = static_cast<float>(((i + 5U) * 29U + (i >> 5U)) % 263U) / 263.0F,
  };
}

inline auto fft_direct_sample(fft_complex const *input, unsigned n, unsigned k) -> fft_complex {
  fft_complex sum{};
  for (unsigned i = 0; i < n; ++i) {
    double angle =
        -2.0 * fft_pi * static_cast<double>((static_cast<std::uint64_t>(i) * k) % n) / static_cast<double>(n);
    fft_complex w = fft_exp(angle);
    sum = sum + input[i] * w;
  }
  return sum;
}

inline auto fft_factor(unsigned n) -> unsigned {
  if (n < 2) {
    return 1;
  }
  if (n == 64 || n == 128 || n == 256 || n == 1024 || n == 2048 || n == 4096) {
    return 8;
  }
  if ((n & 15U) == 0U) {
    return 16;
  }
  if ((n & 7U) == 0U) {
    return 8;
  }
  if ((n & 3U) == 0U) {
    return 4;
  }
  if ((n & 1U) == 0U) {
    return 2;
  }
  for (unsigned r = 3; r < n; r += 2) {
    if (n % r == 0) {
      return r;
    }
  }
  return n;
}

inline auto fft_make(unsigned exponent) -> fft_problem {
  unsigned n = 1U << exponent;
  fft_problem problem{
      .input = std::make_unique<fft_complex[]>(n),
      .work = std::make_unique<fft_complex[]>(n),
      .output = std::make_unique<fft_complex[]>(n),
      .roots = std::make_unique<fft_complex[]>(n),
      .expected = {},
      .n = n,
  };

  for (unsigned i = 0; i < n; ++i) {
    problem.input[i] = fft_input_value(i);
  }
  for (unsigned k = 0; k < n / 2; ++k) {
    double angle = -2.0 * fft_pi * static_cast<double>(k) / static_cast<double>(n);
    problem.roots[k] = fft_exp(angle);
  }
  for (unsigned k = n / 2; k < n; ++k) {
    double angle = -2.0 * fft_pi * static_cast<double>(k) / static_cast<double>(n);
    problem.roots[k] = fft_exp(angle);
  }
  for (std::size_t i = 0; i < fft_check_indices.size(); ++i) {
    problem.expected[i] = fft_direct_sample(problem.input.get(), n, fft_check_indices[i] % n);
  }

  return problem;
}

inline void fft_small_rec(fft_complex const *in, unsigned stride, fft_complex *out, unsigned n) {
  if (n == 1) {
    out[0] = in[0];
    return;
  }

  unsigned half = n / 2;
  fft_small_rec(in, stride * 2, out, half);
  fft_small_rec(in + stride, stride * 2, out + half, half);

  for (unsigned k = 0; k < half; ++k) {
    double angle = -2.0 * fft_pi * static_cast<double>(k) / static_cast<double>(n);
    fft_complex even = out[k];
    fft_complex odd = fft_exp(angle) * out[k + half];
    out[k] = even + odd;
    out[k + half] = even - odd;
  }
}

inline void fft_base_kernel(fft_complex const *in, fft_complex *out, unsigned n) {
  fft_small_rec(in, 1, out, n);
}

inline void fft_unshuffle_range(
    unsigned begin, unsigned end, fft_complex const *in, fft_complex *out, unsigned r, unsigned m) {
  for (unsigned i = begin; i < end; ++i) {
    fft_complex const *src = in + static_cast<std::size_t>(i) * r;
    fft_complex *dst = out + i;
    for (unsigned j = 0; j < r; ++j) {
      *dst = src[j];
      dst += m;
    }
  }
}

inline void fft_unshuffle_serial(
    unsigned begin, unsigned end, fft_complex const *in, fft_complex *out, unsigned r, unsigned m) {
  if (end - begin < fft_unshuffle_cutoff) {
    fft_unshuffle_range(begin, end, in, out, r, m);
    return;
  }

  unsigned mid = begin + (end - begin) / 2;
  fft_unshuffle_serial(begin, mid, in, out, r, m);
  fft_unshuffle_serial(mid, end, in, out, r, m);
}

inline void fft_twiddle_range(unsigned begin,
                              unsigned end,
                              fft_complex const *in,
                              fft_complex *out,
                              fft_complex const *roots,
                              unsigned n_roots,
                              unsigned root_step,
                              unsigned r,
                              unsigned m) {
  for (unsigned i = begin; i < end; ++i) {
    for (unsigned k = 0; k < r; ++k) {
      fft_complex sum{};
      unsigned root_delta = root_step * (i + m * k);
      unsigned root = 0;
      for (unsigned j = 0; j < r; ++j) {
        sum = sum + in[static_cast<std::size_t>(j) * m + i] * roots[root];
        root += root_delta;
        if (root >= n_roots) {
          root %= n_roots;
        }
      }
      out[static_cast<std::size_t>(k) * m + i] = sum;
    }
  }
}

inline void fft_twiddle_serial(unsigned begin,
                               unsigned end,
                               fft_complex const *in,
                               fft_complex *out,
                               fft_complex const *roots,
                               unsigned n_roots,
                               unsigned root_step,
                               unsigned r,
                               unsigned m) {
  if (end - begin < fft_twiddle_cutoff) {
    fft_twiddle_range(begin, end, in, out, roots, n_roots, root_step, r, m);
    return;
  }

  unsigned mid = begin + (end - begin) / 2;
  fft_twiddle_serial(begin, mid, in, out, roots, n_roots, root_step, r, m);
  fft_twiddle_serial(mid, end, in, out, roots, n_roots, root_step, r, m);
}

inline void
fft_mixed_serial(fft_complex *in, fft_complex *out, fft_complex const *roots, unsigned n_roots, unsigned n) {
  if (n <= fft_base_cutoff) {
    fft_base_kernel(in, out, n);
    return;
  }

  unsigned r = fft_factor(n);
  unsigned m = n / r;
  if (r < n) {
    fft_unshuffle_serial(0, m, in, out, r, m);
    for (unsigned k = 0; k < n; k += m) {
      fft_mixed_serial(out + k, in + k, roots, n_roots, m);
    }
  }
  fft_twiddle_serial(0, m, in, out, roots, n_roots, n_roots / n, r, m);
}

inline auto fft_is_close(fft_output out, double tolerance) -> bool {
  double error = 0.0;
  for (std::size_t i = 0; i < fft_check_indices.size(); ++i) {
    unsigned k = fft_check_indices[i] % out.n;
    auto diff = out.output[k] - (*out.expected)[i];
    double scale = std::max(fft_abs((*out.expected)[i]), 1.0);
    error = std::max(error, fft_abs(diff) / scale);
  }
  return error <= tolerance;
}

template <typename Fn>
void run_fft(benchmark::State &state, std::int64_t threads, Fn fn) {
  auto exponent = static_cast<unsigned>(state.range(0));
  auto problem = fft_make(exponent);

  state.counters["n"] = problem.n;
  state.counters["cutoff"] = fft_base_cutoff;

  lf_bench::bench(state, threads, 2e-3, fft_is_close, [&]() -> fft_output {
    state.PauseTiming();
    std::copy_n(problem.input.get(), problem.n, problem.work.get());
    state.ResumeTiming();

    std::invoke(fn, problem.work.get(), problem.output.get(), problem.roots.get(), problem.n);
    return {.output = problem.output.get(), .expected = &problem.expected, .n = problem.n};
  });
}

template <typename Fn>
void run_fft(benchmark::State &state, Fn fn) {
  run_fft(state, lf_bench::no_threads, fn);
}
