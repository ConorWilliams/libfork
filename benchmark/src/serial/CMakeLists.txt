add_executable(serial_benchmark fib.cpp uts.cpp)

target_link_libraries(serial_benchmark
  PRIVATE
    benchmark_common
    benchmark::benchmark_main
)

if(BUILD_TESTING)
  add_test(NAME Benchmark_serial
    COMMAND serial_benchmark --benchmark_dry_run --benchmark_filter=^test/
  )
endif()
