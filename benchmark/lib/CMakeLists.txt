add_library(benchmark_common)

target_compile_features(benchmark_common PUBLIC cxx_std_26)

target_sources(benchmark_common
  PRIVATE
    uts.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      bench.hpp
      cholesky.hpp
      fib.hpp
      fft.hpp
      fold.hpp
      heat.hpp
      integrate.hpp
      knapsack.hpp
      lu.hpp
      macros.hpp
      mandelbrot.hpp
      mergesort.hpp
      matmul.hpp
      nqueens.hpp
      primes.hpp
      quadrature_integrate.hpp
      rectmul.hpp
      quicksort.hpp
      scan.hpp
      skynet.hpp
      strassen.hpp
      transpose.hpp
      uts.hpp
      winograd.hpp
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/uts external/uts)

target_link_libraries(benchmark_common
  PUBLIC
    benchmark::benchmark
    uts_c
)
