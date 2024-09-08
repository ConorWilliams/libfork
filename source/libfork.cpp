#include <string>

#include "libfork/libfork.hpp"

#include <fmt/core.h>

exported_class::exported_class()
    : m_name {fmt::format("{}", "libfork")}
{
}

auto exported_class::name() const -> char const*
{
  return m_name.c_str();
}
