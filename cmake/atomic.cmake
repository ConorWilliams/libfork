include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#include <atomic>
int main() {
  std::atomic<uint8_t> w1;
  std::atomic<uint64_t> w8;
  return ++w1 + ++w8;
}" NATIVE_ATOMICS_SUPPORTED)

if(NOT NATIVE_ATOMICS_SUPPORTED)
  set(LIBFORK_EXTRA_LIBS ${LIBFORK_EXTRA_LIBS} atomic)
endif()
