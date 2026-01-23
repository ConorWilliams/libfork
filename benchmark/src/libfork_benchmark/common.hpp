#include <exception>

struct result_doesnt_match : public std::exception {
  auto what() const noexcept -> char const * override {
    return "Benchamrk result doesn't match reference value!";
  }
};
