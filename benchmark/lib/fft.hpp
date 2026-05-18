#pragma once

#include <algorithm>

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

inline constexpr unsigned fft_cutoff = 1024;
inline constexpr double fft_pi = 3.141592653589793238462643383279502884;
inline constexpr std::array<unsigned, 8> fft_check_indices = {0, 1, 3, 5, 17, 97, 251, 509};

struct fft_complex {
  double re;
  double im;
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

inline auto fft_abs(fft_complex z) -> double { return std::sqrt(z.re * z.re + z.im * z.im); }

inline auto fft_input_value(unsigned i) -> fft_complex {
  return {
      .re = static_cast<double>(((i + 1U) * 17U + (i >> 3U)) % 257U) / 257.0,
      .im = static_cast<double>(((i + 5U) * 29U + (i >> 5U)) % 263U) / 263.0,
  };
}

inline auto fft_direct_sample(fft_complex const *input, unsigned n, unsigned k) -> fft_complex {
  fft_complex sum{};
  for (unsigned i = 0; i < n; ++i) {
    double angle =
        -2.0 * fft_pi * static_cast<double>((static_cast<std::uint64_t>(i) * k) % n) / static_cast<double>(n);
    fft_complex w{.re = std::cos(angle), .im = std::sin(angle)};
    sum = sum + input[i] * w;
  }
  return sum;
}

inline auto fft_make(unsigned exponent) -> fft_problem {
  unsigned n = 1U << exponent;
  fft_problem problem{
      .input = std::make_unique<fft_complex[]>(n),
      .work = std::make_unique<fft_complex[]>(n),
      .output = std::make_unique<fft_complex[]>(n),
      .roots = std::make_unique<fft_complex[]>(n / 2),
      .expected = {},
      .n = n,
  };

  for (unsigned i = 0; i < n; ++i) {
    problem.input[i] = fft_input_value(i);
  }
  for (unsigned k = 0; k < n / 2; ++k) {
    double angle = -2.0 * fft_pi * static_cast<double>(k) / static_cast<double>(n);
    problem.roots[k] = {.re = std::cos(angle), .im = std::sin(angle)};
  }
  for (std::size_t i = 0; i < fft_check_indices.size(); ++i) {
    problem.expected[i] = fft_direct_sample(problem.input.get(), n, fft_check_indices[i] % n);
  }

  return problem;
}

inline void fft_serial_rec(fft_complex const *in,
                           unsigned stride,
                           fft_complex *out,
                           unsigned n,
                           fft_complex const *roots,
                           unsigned root_step) {
  if (n == 1) {
    out[0] = in[0];
    return;
  }

  unsigned half = n / 2;
  fft_serial_rec(in, stride * 2, out, half, roots, root_step * 2);
  fft_serial_rec(in + stride, stride * 2, out + half, half, roots, root_step * 2);

  for (unsigned k = 0; k < half; ++k) {
    fft_complex even = out[k];
    fft_complex odd = roots[k * root_step] * out[k + half];
    out[k] = even + odd;
    out[k + half] = even - odd;
  }
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
  state.counters["cutoff"] = fft_cutoff;

  lf_bench::bench(state, threads, 1e-8, fft_is_close, [&]() -> fft_output {
    std::invoke(fn, problem.input.get(), problem.output.get(), problem.roots.get(), problem.n);
    return {.output = problem.output.get(), .expected = &problem.expected, .n = problem.n};
  });
}

template <typename Fn>
void run_fft(benchmark::State &state, Fn fn) {
  run_fft(state, lf_bench::no_threads, fn);
}
