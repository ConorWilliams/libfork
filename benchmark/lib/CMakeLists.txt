add_library(benchmark_common)

target_compile_features(benchmark_common PUBLIC cxx_std_26)

target_sources(benchmark_common
  PRIVATE
    uts.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
      common.hpp
      fib.hpp
      uts.hpp
      macros.hpp
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/uts external/uts)

target_link_libraries(benchmark_common
  PUBLIC
    benchmark::benchmark
    uts_c
)
