export module libfork.core:constants;

import std;

namespace lf {

constexpr std::size_t k_kilobyte = 1024;
constexpr std::size_t k_megabyte = 1024 * k_kilobyte;

constexpr std::uint32_t k_u16_max = std::numeric_limits<std::uint16_t>::max();

export constexpr std::size_t k_new_align = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
export constexpr std::size_t k_cache_line = std::hardware_destructive_interference_size;

} // namespace lf
