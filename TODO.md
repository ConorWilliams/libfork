# The todo list

- Get lazy pool working.
- Full test suite working.
- co_alloc working -> std:optional<std::span<>>
  - Which needs lf::or_alloc

- non_null macro

- Includes for std:: and lf:: need tidying
- Needs headers in the files
- Docs need updating
- Readme needs updating

- Benchmarks working, run benchmarks on CSD3 with mimalloc

## Features

- [ ] Make busy pool API same as lazy pool.
- [ ] reduce algorithm.

## Misc

- [ ] check comments for broken formatting.
- [ ] review: `std::` includes in schedule/root.
- [ ] review: `libfork/` includes.

## Docs

Main README:

- [ ] README.md: explicit scheduling.
- [ ] README.md: `co_await resume_on(...)`.
- [ ] README.md: compiler explorer with `single_header.hpp`
- [ ] README.md: fix links to docs

Other:

- [ ] Add vcpkg to hacking/building.
- [ ] Add recursive clone to hacking/building.
- [ ] Catalog deps for compilation (i.e pkg-config) (fresh ubuntu wsl).
- [ ] Benchmarks README.md
- [ ] Release notes/changes.

## Future

- [ ] CI: `-fno-exceptions` test.
- [ ] CI: check `single_header.hpp` is up to date.
- [ ] `lf::tail`.
- [ ] Stack-tracing: Logging at call-site (`std::source_location`).
- [ ] Stack-tracing: Walk stack function.
- [ ] Stack-tracing: Signal handler.
- [ ] Detect stack overflows in debug mode.
- [ ] `scan` algorithm.
