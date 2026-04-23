add_library(baremetal_benchmarks OBJECT fib.cpp)

target_link_libraries(baremetal_benchmarks PUBLIC benchmark_common)
