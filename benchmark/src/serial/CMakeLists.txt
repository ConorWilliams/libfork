add_library(serial_benchmarks)

target_sources(serial_benchmarks PRIVATE fib.cpp uts.cpp)

target_link_libraries(serial_benchmarks PUBLIC benchmark_common)
