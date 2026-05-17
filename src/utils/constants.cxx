export module libfork.utils:constants;

import std;

namespace lf {

export constexpr std::size_t k_kilobyte = 1024;
export constexpr std::size_t k_megabyte = 1024 * k_kilobyte;

export constexpr std::uint32_t k_u16_max = std::numeric_limits<std::uint16_t>::max();

export constexpr std::size_t k_new_align = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
export constexpr std::size_t k_page_size = 4096; // 4 KiB on most systems.

#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Winterference-size"
#endif // #ifdef __GNUC__
export constexpr std::size_t k_cache_line = std::hardware_destructive_interference_size;
#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
  #pragma GCC diagnostic pop
#endif // #ifdef __GNUC__

} // namespace lf
