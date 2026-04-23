add_executable(libfork_benchmark fib.cpp uts.cpp)

target_link_libraries(libfork_benchmark
  PRIVATE
    benchmark_common
    benchmark::benchmark_main
)

if(BUILD_TESTING)
  add_test(NAME Benchmark_libfork
    COMMAND libfork_benchmark --benchmark_dry_run --benchmark_filter=^test/
  )
endif()
