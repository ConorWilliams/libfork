# The todo list

1. Complete Core:
      - [x] Complete test suit for core.
      - [x] Tidy up propagate exception macros.
      - [x] Where we can use `LF_HOF_RETURNS`
      - [x] Update licenses at top of files.
      - [x] Touch up `std::` includes.
      - [x] Redo name spacing (rm `DEPENDANT_ABI`, `user`, `detail`, `impl`, and `schedule`).
      - [ ] A test that stresses stack stealing?

2. Get the docs building and looking good again.
      - [x] Rename `DOXYGEN_SHOULD_SKIP_THIS` to `DOXYGEN_PROCESSOR`.
      - [x] Document macros.

3. Get it compiling on CI:
      - [x] Remove CPM.cmake
      - [x] Try core test suit on compiler explorer, fix constexpr requirements in i.e. `byte_cast`.
      - [x] vcpkg for tests.
      - [x] Get CI working.
      - [ ] `-fno-exceptions` test.
      - [ ] check `single_header.hpp` is up to date.

4. Review: 
      - [x] `constexpr`.
      - [ ] `noexcept`.
      - [ ] `std::` includes in schedule/root.

5. Core features + tests for them:
      - [x] `co_await resume_on(...)`.
      - [ ] `lf::tail`.

6. Schedulers:
      - [x] Stack stealing.
      - [x] busy_pool.
      - [ ] Sleepy scheduler.
      - [ ] NUMA aware.

7. Benchmarks 
      - [x] Compiling.
      - [ ] TaskFlow.
      - [ ] Graphs.
      - [ ] More heterogenous workload (UT3, unbalanced tree reduce)
      - [ ] Review 30% branch miss-predict, seems to be related to spinning workers?

8. Documentation:
      - [ ] Add vcpkg to hacking/building.
      - [ ] Add recursive clone to hacking/building.
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
      - [ ] Modules (cpp20) `support`.



Naming:

silk -> cilk
wool
weave 
lace
fibril
nowa
taskflow 















