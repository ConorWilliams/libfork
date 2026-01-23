#pragma once

#include <format>
#include <stdexcept>

#include "libfork/macros.hpp"

/**
 * @class incorrect_result
 * @brief A wrapper around std::runtime_error to indicate an incorrect result.
 *
 */
struct incorrect_result : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

#define CHECK_RESULT(result, expected)                                                                       \
  do {                                                                                                       \
    if ((result) != (expected)) {                                                                            \
      LF_THROW(incorrect_result(std::format("{}={} != {}={}", #expected, (expected), #result, (result))));   \
    }                                                                                                        \
  } while (0)
