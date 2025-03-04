#pragma once

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <print>

#define LF_LOG(...)                                                                                \
  do {                                                                                             \
  } while (0)

#ifndef LF_LOG
  #define LF_LOG(fmt, ...) std::println(fmt __VA_OPT__(, ) __VA_ARGS__)
#endif
