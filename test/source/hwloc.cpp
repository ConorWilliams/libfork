

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <errno.h>
#include <stdexcept>
#include <stdio.h>
#include <string.h>

#include <catch2/catch_test_macros.hpp>

#include "libfork/core.hpp"

#ifdef LF_HAS_HWLOC

  #include <hwloc.h>

// struct hwloc_tree {
//     int depth
// };

static void print_children(hwloc_obj_t obj, int depth) {

  char type[32], attr[1024];
  unsigned i;

  //   obj->

  hwloc_obj_type_snprintf(type, sizeof(type), obj, 0);

  printf("%*s%s", 2 * depth, "", type);

  if (obj->os_index != (unsigned)-1) {
    printf("#%u", obj->os_index);
  }

  hwloc_obj_attr_snprintf(attr, sizeof(attr), obj, " ", 0);

  //   if (*attr) {
  //     printf("(%s)", attr);
  //   }
  printf("\n");

  for (i = 0; i < obj->arity; i++) {
    print_children(obj->children[i], depth + 1);
  }
}

TEST_CASE("hwloc", "[hwloc]") {

  hwloc_topology_t topology = nullptr;

  lf::impl::defer at_exit = [&topology]() noexcept {
    if (topology != nullptr) {
      hwloc_topology_destroy(topology);
    }
  };

  if (hwloc_topology_init(&topology) != 0) {
    throw std::runtime_error{"hwloc failed to initialize a topology"};
  }

  if (hwloc_topology_load(topology) != 0) {
    throw std::runtime_error{"hwloc failed to load a topology"};
  }

  print_children(hwloc_get_root_obj(topology), 0);
}

#endif