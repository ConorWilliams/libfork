add_library(serial_benchmarks)

target_sources(serial_benchmarks PRIVATE
  fib.cpp
  fold.cpp
  heat.cpp
  integrate.cpp
  knapsack.cpp
  mandelbrot.cpp
  matmul.cpp
  nqueens.cpp
  primes.cpp
  quicksort.cpp
  scan.cpp
  skynet.cpp
  strassen.cpp
  uts.cpp
)

target_link_libraries(serial_benchmarks PUBLIC benchmark_common)
