#!/bin/bash

lint_dir() {

  local files=$(find "$1" \( -name '*.cpp' -o -name '*.hpp' \) -not -path "./.*")

  for file in $files; do
    if [ "${FIX:-false}" = "true" ]; then
      clang-format -i "$file"
    else
      clang-format --dry-run --Werror "$file"
    fi
  done
}

lint_dir "test"
lint_dir "src"
lint_dir "include"
