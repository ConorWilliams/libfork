add_library(libfork_benchmarks OBJECT fib.cpp uts.cpp)

target_link_libraries(libfork_benchmarks PUBLIC benchmark_common)
