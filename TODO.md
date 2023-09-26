# The todo list

#### Done :)

- [x] Complete test suit for core.
- [x] Tidy up propagate exception macros.
- [x] Where we can use `LF_HOF_RETURNS`
- [x] Update licenses at top of files.
- [x] Touch up `std::` includes.
- [x] Redo name spacing (rm `DEPENDANT_ABI`, `user`, `detail`, `impl`, and `schedule`).
- [x] Document macros.
- [x] Remove CPM.cmake
- [x] Try core test suit on compiler explorer, fix constexpr requirements in i.e. `byte_cast`.
- [x] vcpkg for tests.
- [x] Get CI working.
- [x] `co_await resume_on(...)`.
- [x] Stack stealing.
- [x] `busy_pool.
- [x] Optimize single-threaded contexts
- [x] Separate invoke awaitable from promise.
- [x] Remove `frame_block` from public api's
- [x] Works-around <https://github.com/llvm/llvm-project/issues/63022>
- [x] Numa primitives (hwloc).
- [x] `lazy_pool`
- [x] `numa_pool` (will need to iterate on this once we have heterogeneous workloads).

#### Misc

#### Post feature complete

- [ ] add numa mode control
- [ ] benchmark: need some heterogeneous workloads (UT3, unbalanced tree reduce, see NOWA)
- [ ] benchmark: A test/benchmark that stresses stack stealing?
- [ ] benchmark: C++ library comparisons (TaskFlow, ...).
- [ ] benchmark: Memory allocations of our stack stealing.
- [ ] Review 30% branch miss-predict, seems to be related to spinning workers?
- [ ] Graphs/analysis.

#### Before release

- [x] review: `constexpr`.
- [x] review: `noexcept`.
- [ ] review: `std::` includes in schedule/root.
- [ ] review: `libfork/` includes
- [x] docs: complete extension docs.
- [ ] docs: Add vcpkg to hacking/building.
- [ ] docs: Add recursive clone to hacking/building.
- [ ] docs: Catalog deps for compilation (i.e pkg-config) (fresh ubuntu wsl).
- [ ] docs: Update readme.
- [ ] docs: Add `single_header.hpp` to readme + link to compiler explorer.
- [ ] docs: Release notes/changes.

#### Stretch

- [ ] CI: `-fno-exceptions` test.
- [ ] CI: check `single_header.hpp` is up to date.
- [ ] `lf::tail`.
- [ ] Stack-tracing: Logging at call-site (`std::source_location`).
- [ ] Stack-tracing: Walk stack function.
- [ ] Stack-tracing: Signal handler.

#### Future

Algorithms:

- [ ] `for_each`.
- [ ] `reduce`.
- [ ] `scan`.

Modules (cpp20) support.

Naming:

silk -> cilk
wool
weave
lace
fibril
nowa
taskflow
