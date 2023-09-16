# The todo list

1. Complete Core:
      - [x] Complete test suit for core.
      - [x] Tidy up propagate exception macros.
      - [x] Where we can use `LF_HOF_RETURNS`
      - [x] Update licenses at top of files.
      - [x] Touch up `std::` includes.
      - [ ] Redo name spacing (rm `DEPENDANT_ABI`, `user`, `detail`, `impl`, and `schedule`).

2. Get the docs building and looking good again.
      - [ ] Rename `DOXYGEN_SHOULD_SKIP_THIS` to `DOXYGEN_PROCESSOR`.

3. Get it compiling on CI:
      - [ ] Remove CPM.cmake
      - [ ] Try core test suit on compiler explorer, fix constexpr requirements in i.e. `byte_cast`.
      - [ ] vcpkg for tests.
      - [ ] `-fno-exceptions` test.
      - [ ] check `single_header.hpp` is up to date.

4. Review: 
      - [ ] `noexcept`/`constexpr`.

5. Core features + tests for them:
      - [ ] `co_await resume_on(...)`.
      - [ ] `lf::tail`.

6. Schedulers:
      - [ ] Stack stealing.
      - [ ] Sleepy scheduler.
      - [ ] NUMA aware.

7. Benchmarks 
      - [ ] Compiling.
      - [ ] TaskFlow.
      - [ ] Graphs.

8. Documentation:
      - [ ] Add vcpkg to hacking/building.
      - [ ] Catalog deps for compilation (i.e pkg-config) (fresh ubuntu wsl).
      - [ ] Update readme.

9. Stack-tracing api:
      - [ ] Logging at call-site (`std::source_location`).
      - [ ] Walk stack function.
      - [ ] Signal handler.

10. Algorithms: 
      - [ ] `for_each`.
      - [ ] `reduce`.
      - [ ] `scan`.

11. Stretch:
      - [ ] Module `support`.



Naming:

silk -> cilk
wool
weave 
lace
fibril
nowa
taskflow 















