# The todo list

1. Complete Core:
      1. Complete test suit for core.
      2. Tidy up propagate exception macros.
      3. Update licenses at top of files.
      4. Touch up `std::` includes.
      5. Redo name spacing (rm `DEPENDANT_ABI`, `user`, `detail`, `impl`, and `schedule`).
      6. Where we can use `LF_HOF_RETURNS`

2. Get the docs building and looking good again.
      1. Rename `DOXYGEN_SHOULD_SKIP_THIS` to `DOXYGEN_PROCESSOR`.

3. Get it compiling on CI:
      1. Try core test suit on compiler explorer, fix constexpr requirements in i.e. `byte_cast`.
      2. vcpkg for tests
      3. `-fno-exceptions` test
      4. check `single_header.hpp` is up to date.

4. Review `noexcept`/`constexpr`

5. Core features + tests for them:
      1. `co_await resume_on(...)`;
      2. `lf::tail`
      3. Logging at call-site (`std::source_location`) + stack tracing api

6. NUMA aware scheduler

7. Benchmarks (and task-flow)

8. Documentation:
      1. Add vcpkg to hacking/building
      2. Catalog deps for compilation (i.e pkg-config) (fresh ubuntu wsl)
      3. Update readme

9. Algorithms: 
      1. `for_each`
      2. `reduce`
      3. `scan`



Naming:

silk -> cilk
wool
weave 
lace
fibril
nowa
taskflow 















