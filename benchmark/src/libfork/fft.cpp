#include <benchmark/benchmark.h>

#include "fft.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

struct fft_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>,
                         fft_complex const *in,
                         unsigned stride,
                         fft_complex *out,
                         unsigned n,
                         fft_complex const *roots,
                         unsigned root_step) -> lf::task<void, Context> {
    if (n <= fft_cutoff) {
      fft_serial_rec(in, stride, out, n, roots, root_step);
      co_return;
    }

    unsigned half = n / 2;
    {
      auto sc = co_await lf::scope();
      co_await sc.fork(fft_task{}, in, stride * 2, out, half, roots, root_step * 2);
      co_await sc.call(fft_task{}, in + stride, stride * 2, out + half, half, roots, root_step * 2);
      co_await sc.join();
    }

    for (unsigned k = 0; k < half; ++k) {
      fft_complex even = out[k];
      fft_complex odd = roots[k * root_step] * out[k + half];
      out[k] = even + odd;
      out[k + half] = even - odd;
    }
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);

  run_fft(state,
          threads,
          [&](fft_complex const *input, fft_complex *output, fft_complex const *roots, unsigned n) {
            lf::schedule(scheduler, fft_task{}, input, 1U, output, n, roots, 1U).get();
          });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, fft, fft, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, fft, fft, poly_busy_pool)
