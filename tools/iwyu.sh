#!/bin/bash

for f in $(find include/libfork/core/ -iname *.hpp -type f); do
    include-what-you-use  -Xiwyu --max_line_length=100 -Xiwyu --update_comments -Xiwyu --no_fwd_decls -Xiwyu --cxx17ns -I./include -std=c++23 $f 2> /tmp/iwyu.out
    fix_includes.py --comments --update_comments < /tmp/iwyu.out
done

for f in $(find include/libfork/schedule/ -iname *.hpp -type f); do
    include-what-you-use  -Xiwyu --max_line_length=100 -Xiwyu --update_comments -Xiwyu --no_fwd_decls -Xiwyu --cxx17ns -I./include -std=c++23 $f 2> /tmp/iwyu.out
    fix_includes.py --comments --update_comments < /tmp/iwyu.out
done

for f in $(find include/libfork/algorithm/ -iname *.hpp -type f); do
    include-what-you-use  -Xiwyu --max_line_length=100 -Xiwyu --update_comments -Xiwyu --no_fwd_decls -Xiwyu --cxx17ns -I./include -std=c++23 $f 2> /tmp/iwyu.out
    fix_includes.py --comments --update_comments < /tmp/iwyu.out
done

for f in $(find test/source -iname *.?pp -type f); do
    iwyu_tool.py -p build/dev/compile_commands.json $f -- -Xiwyu --mapping_file=$PWD/tools/iwyu.map.json -Xiwyu --max_line_length=100 -Xiwyu --update_comments -Xiwyu --no_fwd_decls -Xiwyu --cxx17ns 1> /tmp/iwyu.out
    fix_includes.py --comments --update_comments < /tmp/iwyu.out
done