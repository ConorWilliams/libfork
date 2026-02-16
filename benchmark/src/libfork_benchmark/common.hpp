#pragma once

#include <format>
#include <stdexcept>

#include "libfork/__impl/exception.hpp"

struct incorrect_result : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

#define CHECK_RESULT(result, expected)                                                                       \
  do {                                                                                                       \
    auto &&lf_check_result_val = (result);                                                                   \
    auto &&lf_check_expected_val = (expected);                                                               \
    if (lf_check_result_val != lf_check_expected_val) {                                                      \
      LF_THROW(incorrect_result(                                                                             \
          std::format("{}={} != {}={}", #expected, lf_check_expected_val, #result, lf_check_result_val)));   \
    }                                                                                                        \
  } while (0)
