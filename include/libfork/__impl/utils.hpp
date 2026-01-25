#pragma once

/**
 * @file utils.hpp
 *
 * @brief A collection of internal utility macros.
 *
 * These macros are not safe to use unless `import std` is in scope.
 */

// =============== Utility =============== //

// clang-format off

/**
 * @brief Use like `BOOST_HOF_RETURNS` to define a function/lambda with all the noexcept/decltype specifiers.
 *
 * This macro is not truly variadic but the ``...`` allows commas in the macro argument.
 */
#define LF_HOF(...) noexcept(noexcept(__VA_ARGS__)) -> decltype(__VA_ARGS__) { return __VA_ARGS__;}

// clang-format on

/**
 * @brief Use like `std::forward` to perfectly forward an expression.
 */
#define LF_FWD(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
