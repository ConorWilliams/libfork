#include <benchmark/benchmark.h>

#include "primes.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

inline constexpr std::int64_t primes_chunk = 4096;

struct prime_count {
  auto operator()(std::int64_t value) const -> std::int64_t { return is_prime(value) ? 1 : 0; }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);
  lf_bench::report_threads(state, threads);

  run_primes(state, [&](std::int64_t lim) -> std::int64_t {
    auto values = std::views::iota(std::int64_t{2}, lim);
    return *lf::schedule(scheduler, lf::fold, values, primes_chunk, std::plus<>{}, prime_count{}).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, primes, primes, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, primes, primes, poly_busy_pool)
