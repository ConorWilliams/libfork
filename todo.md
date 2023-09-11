

- split docs into: User api, Context API (for implementors), Internal + detail for no-doc
- Catalog deps (i.e pkg-config) (fresh ubuntu wsl)
- detect HALO and bail out
- invoke (eventually<T>, because exceptions don't propagate return will never have thrown!) 
- logging at call-site, name at definition task<T, "name">, can make static_string constructor accept a defaulted soucre_location
- pass return_address in first_arg
- no forked r-values at the promise constructor level
- test dependencies when installing, test install on fresh machine.
- CI -fno-exceptions
- non-null make a strong type
- tail/invoke
- remove constexpr in byte_cast and friends (MSVC showed us the way)
- A check in the CI that makes sure single header is up to date with the rest of the code.
- tidy up propagete exception macros
- remove DEPENDANT_ABI namespace.
- Update licenses at top of files
- Taskflow benchmark

Naming:

silk -> cilk
wool
weave 
lace
fibril
nowa
taskflow 


Notes:

ORDER THIS LIST


1. Migration:
      - Move to c++23
      - ADD an -fno-exceptions compilation test.

2. Code changes
      - TRY/CATCH macros
      - terminate_with(lambda) function tagged noexcept
      - make thread_local.hpp private / remove from API -> move into schedule, add a master schedule.hpp
      - Update includes (prune exess std)
      - [[nodiscard]],constexpr,noexcept double checking
      
3. Code additions:
      - A good scheduler that uses HWLOC
      - Parallel functions: fold, for_each, scan















