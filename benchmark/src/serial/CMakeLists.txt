add_library(serial_benchmarks OBJECT fib.cpp uts.cpp)

target_link_libraries(serial_benchmarks PUBLIC benchmark_common)
