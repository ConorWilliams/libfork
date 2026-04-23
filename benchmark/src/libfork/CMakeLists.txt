add_library(libfork_benchmarks)

target_sources(libfork_benchmarks PRIVATE fib.cpp uts.cpp)

target_link_libraries(libfork_benchmarks PUBLIC benchmark_common)
