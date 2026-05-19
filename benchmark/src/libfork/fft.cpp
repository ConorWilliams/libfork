#include <benchmark/benchmark.h>

#include "fft.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct fft_unshuffle_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         unsigned begin,
                         unsigned end,
                         fft_complex const *in,
                         fft_complex *out,
                         unsigned r,
                         unsigned m) -> lf::task<void, Context> {
    if (end - begin < fft_unshuffle_cutoff) {
      fft_unshuffle_range(begin, end, in, out, r, m);
      co_return;
    }

    unsigned mid = begin + (end - begin) / 2;
    auto sc = co_await lf::scope();
    co_await sc.fork(fft_unshuffle_task{}, begin, mid, in, out, r, m);
    co_await sc.call(fft_unshuffle_task{}, mid, end, in, out, r, m);
    co_await sc.join();
  }
};

struct fft_twiddle_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         unsigned begin,
                         unsigned end,
                         fft_complex const *in,
                         fft_complex *out,
                         fft_complex const *roots,
                         unsigned n_roots,
                         unsigned root_step,
                         unsigned r,
                         unsigned m) -> lf::task<void, Context> {
    if (end - begin < fft_twiddle_cutoff) {
      fft_twiddle_range(begin, end, in, out, roots, n_roots, root_step, r, m);
      co_return;
    }

    unsigned mid = begin + (end - begin) / 2;
    auto sc = co_await lf::scope();
    co_await sc.fork(fft_twiddle_task{}, begin, mid, in, out, roots, n_roots, root_step, r, m);
    co_await sc.call(fft_twiddle_task{}, mid, end, in, out, roots, n_roots, root_step, r, m);
    co_await sc.join();
  }
};

struct fft_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         unsigned n,
                         fft_complex *in,
                         fft_complex *out,
                         fft_complex const *roots,
                         unsigned n_roots) -> lf::task<void, Context> {
    if (n <= fft_base_cutoff) {
      fft_base_kernel(in, out, n);
      co_return;
    }

    unsigned r = fft_factor(n);
    unsigned m = n / r;
    if (r < n) {
      {
        auto sc = co_await lf::scope();
        co_await sc.call(fft_unshuffle_task{}, 0U, m, in, out, r, m);
        co_await sc.join();
      }

      {
        auto sc = co_await lf::scope();
        for (unsigned k = 0; k + m < n; k += m) {
          co_await sc.fork(fft_task{}, m, out + k, in + k, roots, n_roots);
        }
        co_await sc.call(fft_task{}, m, out + n - m, in + n - m, roots, n_roots);
        co_await sc.join();
      }
    }

    auto sc = co_await lf::scope();
    co_await sc.call(fft_twiddle_task{}, 0U, m, in, out, roots, n_roots, n_roots / n, r, m);
    co_await sc.join();
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_fft(state, threads, [&](fft_complex *input, fft_complex *output, fft_complex const *roots, unsigned n) {
    lf::schedule(scheduler, fft_task{}, n, input, output, roots, n).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, fft, fft, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, fft, fft, poly_busy_pool)
