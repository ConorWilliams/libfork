#pragma once

#include <print>

#define LF_LOG(...)                                                                                \
  do {                                                                                             \
  } while (0)

#ifndef LF_LOG
  #define LF_LOG(fmt, ...) std::println(fmt __VA_OPT__(, ) __VA_ARGS__)
#endif
