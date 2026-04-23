add_library(benchmark_common STATIC uts.cpp)

target_include_directories(benchmark_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(benchmark_common
  PUBLIC
    benchmark::benchmark
    uts_c
    libfork::libfork
)
