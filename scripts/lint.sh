#!/bin/bash
for file in $(find . -name '*.cpp' -o -name '*.hpp'); do
  if [ "${FIX:-false}" = "true" ]; then
    clang-format -i "$file"
  else
    clang-format --dry-run --Werror "$file"
  fi
done
