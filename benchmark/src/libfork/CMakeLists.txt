add_library(libfork_benchmarks)

target_sources(libfork_benchmarks
  PRIVATE
    fib.cpp
    uts.cpp
    switch_io_pool.cpp
    switch_random.cpp
  PRIVATE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      helpers.hpp
)

target_link_libraries(libfork_benchmarks
  PUBLIC
    benchmark_common
    libfork::libfork
)
