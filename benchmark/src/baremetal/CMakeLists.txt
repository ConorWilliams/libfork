add_executable(baremetal_benchmark fib.cpp)

target_link_libraries(baremetal_benchmark
  PRIVATE
    benchmark_common
    benchmark::benchmark_main
)

if(BUILD_TESTING)
  add_test(NAME Benchmark_baremetal
    COMMAND baremetal_benchmark --benchmark_dry_run --benchmark_filter=^test/
  )
endif()
