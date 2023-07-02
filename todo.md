


Notes:

ORDER THIS LIST


TODO:

1. Migration:
      - Move to c++23
      - separate into static/shared library
      - annotations [export]
      - ADD an -fno-exceptions compilation test.
      - Check that exceptions cross shared lib boundaries. (make exceptions public/outside a class)

2. Code changes
      - TRY/CATCH macros
      - terminate_with(lambda) function tagged noexcept
      - Stack becomes a concrete class
      - use if constexpr PROPAGTE_EXCEPTIONS 
      - Make first_arg_t move_only with a move_only anon member/base class (Check .context of packets is always moved)
      - make thread_local.hpp private / remove from API -> move into schedule, add a master schedule.hpp
      - Update includes (prune exess std)
      - move no_forked_rvalue to algorithm

      - [[nodiscard]],constexpr,noexcept double checking
      
3. Code additions:
      - A good scheduler that uses HWLOC
      - Parallel functions: fold, for_each, scan















