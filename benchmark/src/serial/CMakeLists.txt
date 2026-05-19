add_library(serial_benchmarks)

target_sources(serial_benchmarks PRIVATE
  cholesky.cpp
  fib.cpp
  fft.cpp
  fold.cpp
  heat.cpp
  integrate.cpp
  knapsack.cpp
  lu.cpp
  mandelbrot.cpp
  mergesort.cpp
  matmul.cpp
  nqueens.cpp
  primes.cpp
  quadrature_integrate.cpp
  rectmul.cpp
  quicksort.cpp
  scan.cpp
  skynet.cpp
  strassen.cpp
  transpose.cpp
  uts.cpp
  winograd.cpp
)

target_link_libraries(serial_benchmarks PUBLIC benchmark_common)
