#!/bin/bash

for f in $(find include/libfork/core/ -iname *.hpp -type f); do
    include-what-you-use  -Xiwyu --update_comments -Xiwyu --no_fwd_decls -Xiwyu --cxx17ns -I./include -std=c++23 $f 2> /tmp/iwyu.out
    fix_includes.py --comments --update_comments < /tmp/iwyu.out
done