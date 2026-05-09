if(NOT DEFINED BENCHMARK_EXECUTABLE)
  message(FATAL_ERROR "BENCHMARK_EXECUTABLE is required")
endif()

if(NOT DEFINED BENCHMARK_FILTER)
  message(FATAL_ERROR "BENCHMARK_FILTER is required")
endif()

if(NOT DEFINED BENCHMARK_NAME)
  set(BENCHMARK_NAME "${BENCHMARK_FILTER}")
endif()

execute_process(
  COMMAND "${BENCHMARK_EXECUTABLE}" --benchmark_list_tests "--benchmark_filter=${BENCHMARK_FILTER}"
  RESULT_VARIABLE benchmark_result
  OUTPUT_VARIABLE benchmark_output
  ERROR_VARIABLE benchmark_error
)

if(NOT benchmark_result EQUAL 0)
  message(FATAL_ERROR "Failed to list benchmark ${BENCHMARK_NAME}\n${benchmark_error}")
endif()

string(STRIP "${benchmark_output}" benchmark_output)

if(benchmark_output STREQUAL "")
  message(FATAL_ERROR "Benchmark ${BENCHMARK_NAME} was not found")
endif()
