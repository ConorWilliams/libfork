add_library(baremetal_benchmarks)

target_sources(baremetal_benchmarks PRIVATE fib.cpp)

target_link_libraries(baremetal_benchmarks PUBLIC benchmark_common libfork::libfork)
