---
icon: lucide/list
---

# Benchmarks

This section documents each benchmark family and the common registration
patterns used by the benchmark sources.

The benchmark suite is built on Google Benchmark. Shared family definitions live
in `benchmark/lib/`, while implementation variants live under
`benchmark/src/<implementation>/`.

## Shared Benchmark Loop

The common loop is `lf_bench::bench` in `benchmark/lib/bench.hpp`. It owns the
central Google Benchmark iteration loop, result checking, error reporting, and
`DoNotOptimize` call.

Use the overload without a thread count for serial or single-worker variants:

```cpp
lf_bench::bench(state, expected, [] {
  return run_work();
});
```

Use the overload with a thread count for benchmark registrations whose argument
list includes `p`:

```cpp
lf_bench::bench(state, threads, expected, [] {
  return run_parallel_work();
});
```

The callable is always the last argument. Pass callables by value; benchmark
wrappers copy lambdas rather than forwarding them.

If equality is not the right correctness check, pass a custom predicate before
the workload callable:

```cpp
lf_bench::bench(state, expected, result_is_close, [] {
  return run_work();
});
```

For threaded benchmarks, `bench` also reports:

- `state.counters["p"]`, the thread count.
- `SetComplexityN(p)`, used with the inverse-complexity reporter configured by
  the multi-threaded registration macros.

## Family Wrappers

Each benchmark family should hide input construction, expected-result
calculation, and family-specific counters in a helper in `benchmark/lib/`.

For example, `fib.hpp` exposes `run_fib`:

```cpp
template <typename Fn>
void run_fib(benchmark::State &state, std::int64_t threads, Fn fn);

template <typename Fn>
void run_fib(benchmark::State &state, Fn fn);
```

The implementation-specific source then supplies only the work:

```cpp
void fib_run(benchmark::State &state) {
  run_fib(state, [](std::int64_t n) {
    return fib_impl(n);
  });
}
```

Threaded variants put `threads` before the callable so the workload remains the
last argument:

```cpp
void fib_run(benchmark::State &state) {
  auto threads = state.range(1);
  run_fib(state, threads, [threads](std::int64_t n) {
    return fib_impl(n, threads);
  });
}
```

The same pattern is used by:

- `run_fib` for Fibonacci.
- `run_fold_input` for fold input construction and item reporting.
- `run_heat` for heat-grid construction and convergence checking.
- `run_integrate` for integration bounds and tolerance checking.
- `run_knapsack` for problem generation and optimum checking.
- `run_mandelbrot` for image-size counters and checksum checking.
- `run_matmul` for matrix input construction and relative-error checking.
- `run_nqueens` for board allocation and known solution counts.
- `run_primes` for prime-count reference values.
- `run_quicksort` for input generation and sorted-order checking.
- `run_scan` for scan input construction and tail checking.
- `run_skynet` for depth, leaf-count, and expected sum setup.
- `run_uts` for UTS tree setup and root construction.

Benchmarks that do not need a family wrapper can still call `lf_bench::bench`
directly once they have set their counters and built their expected result.

## Registration Macros

Registration macros live in `benchmark/lib/macros.hpp`. They wrap
`benchmark::RegisterBenchmark`, build the benchmark name, and attach the
appropriate argument sets.

Standard single-argument benchmarks use:

```cpp
BENCH_ONE(bench_fn, category, name, mode, prefix, ...);
BENCH_ALL(bench_fn, category, name, prefix, ...);
```

`BENCH_ONE` registers one size. The size comes from `prefix##_##mode`; for
example, `BENCH_ONE(fib_run, serial, fib, test, fib)` uses `fib_test`.

`BENCH_ALL` registers the `test` and `base` sizes:

```cpp
BENCH_ALL(fib_run, serial, fib, fib);
```

Multi-threaded standard benchmarks use:

```cpp
BENCH_ONE_MT(bench_fn, category, name, mode, prefix, ...);
BENCH_ALL_MT(bench_fn, category, name, prefix, ...);
```

These register argument pairs `{size, p}`. The size still comes from
`prefix##_##mode`; `p` is generated from hardware-supported thread counts:
`1, 2, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96`, stopping once the value exceeds
`std::thread::hardware_concurrency()`.

UTS has separate macros because its first benchmark argument is the thread count
and the tree selection is captured in the registration lambda:

```cpp
UTS_BENCH_ONE(bench_fn, category, mode, tree_name, tree_id, ...);
UTS_BENCH_ALL(bench_fn, category, ...);
UTS_BENCH_ONE_MT(bench_fn, category, mode, tree_name, tree_id, ...);
UTS_BENCH_ALL_MT(bench_fn, category, ...);
```

`UTS_BENCH_ALL` registers the mini, base, and large tree presets. The
multi-threaded form registers the same tree presets for each supported thread
count.

## Names And Template Arguments

Benchmark names are formatted as:

```text
<mode>/<category>/<name>[/template-or-argument-tags]
```

The variadic macro arguments are both template arguments for the benchmark
function and a readable suffix in the benchmark name. Spaces are stripped from
the formatted name, so template-heavy libfork scheduler names remain usable in
Google Benchmark filters.

For example:

```cpp
BENCH_ALL(run, libfork, fib, fib, mono_busy_pool);
```

registers `run<mono_busy_pool>` and gives the benchmark a suffix containing
`mono_busy_pool`.

Family-specific macros can layer on top of the standard macros. Fold uses
`LF_FOLD_BENCH_SIZES` and `LF_FOLD_BENCH_SIZES_MT` to register its chosen input
sizes while still delegating to `BENCH_ONE` and `BENCH_ONE_MT`.
