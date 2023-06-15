// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <chrono>
#include <exception>
#include <stdexcept>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "libfork/core/exception.hpp"

// NOLINTBEGIN

using namespace lf::detail;

#if LIBFORK_COMPILER_EXCEPTIONS

TEST_CASE("exception_packet tests") {

  SECTION("Default construction should not store any exception") {
    exception_packet packet;
    REQUIRE(!packet); // Operator bool should return false
  }

  SECTION("Storing and retrieving an exception") {

    exception_packet packet;

    try {
      throw std::runtime_error("Test exception");
    } catch (...) {
      packet.unhandled_exception();
    }

    REQUIRE(packet); // Operator bool should return true

    try {
      packet.rethrow_if_unhandled();
      FAIL("No exception was thrown");
    } catch (std::runtime_error const &ex) {
      // Discard
    }

    REQUIRE(!packet); // Operator bool should return false
  }

  SECTION("Storing multiple exceptions") {

    exception_packet packet;

    try {
      throw std::runtime_error("A");
    } catch (...) {
      packet.unhandled_exception();
    }

    REQUIRE(packet); // Operator bool should return true

    try {
      throw std::runtime_error("B");
    } catch (...) {
      packet.unhandled_exception();
    }

    REQUIRE(packet); // Operator bool should return true

    try {
      packet.rethrow_if_unhandled();
      FAIL("No exception was thrown");
    } catch (std::runtime_error const &ex) {
      REQUIRE(ex.what() == std::string("A"));
    }

    REQUIRE(!packet); // Operator bool should return false
  }

  SECTION("Multiple threads calling unhandled_exception() simultaneously") {

    unsigned int k_threads = std::thread::hardware_concurrency();
    unsigned int k_iter = 100;

    exception_packet packet;
    std::vector<std::thread> threads;

    // Define a lambda function that throws an exception and calls unhandled_exception()
    auto threadFunc = [&]() {
      for (unsigned int i = 0; i < k_iter; ++i) {

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        try {
          throw std::runtime_error("Test exception");
        } catch (...) {
          packet.unhandled_exception();
        }
      }
    };

    // Start multiple threads
    for (unsigned int i = 0; i < k_threads; ++i) {
      threads.emplace_back(threadFunc);
    }

    // Wait for all threads to finish
    for (auto &thread : threads) {
      thread.join();
    }

    REQUIRE(packet); // Operator bool should return true

    try {
      packet.rethrow_if_unhandled();
      FAIL("No exception was thrown");
    } catch (std::runtime_error const &ex) {
      // Handle the exception if needed
      // ...
    }

    REQUIRE(!packet); // Operator bool should return false
  }
}

#endif // LIBFORK_COMPILER_EXCEPTIONS

// NOLINTEND
