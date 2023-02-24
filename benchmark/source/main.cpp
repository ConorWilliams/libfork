auto benchmark_fib() -> void;
auto benchmark_dfs() -> void;

auto main() -> int {
  benchmark_dfs();
  benchmark_fib();
  return 0;
}