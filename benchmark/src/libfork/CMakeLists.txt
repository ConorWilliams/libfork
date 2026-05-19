add_library(libfork_benchmarks)

target_sources(libfork_benchmarks
  PRIVATE
    cholesky.cpp
    fib.cpp
    fft.cpp
    fold.cpp
    heat.cpp
    integrate.cpp
    knapsack.cpp
    lu.cpp
    matmul.cpp
    mergesort.cpp
    nqueens.cpp
    skynet.cpp
    strassen.cpp
    quadrature_integrate.cpp
    quicksort.cpp
    rectmul.cpp
    transpose.cpp
    uts.cpp
    winograd.cpp
    switch_io_pool.cpp
    switch_random.cpp
  PRIVATE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      helpers.hpp
)

target_link_libraries(libfork_benchmarks
  PUBLIC
    benchmark_common
    libfork::libfork
)
