---
icon: lucide/hash
---

# Primes

The primes benchmark counts primes below a limit using trial division. The
predicate uses the standard `6k +/- 1` candidate pattern after handling small
and even divisors.

Source:

- [shared prime helpers](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/primes.hpp)
- [serial implementation](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/primes.cpp)

## What It Measures

`test` counts primes below `100000`; `base` counts primes below `10000000`.
Known prime-counting values are used for those configured sizes.

## Scaling

A parallel version can partition the candidate range across workers. Static
equal-size ranges are not perfectly balanced because larger candidates require
more trial divisions on average, and primes do more work than composites that
hit a small divisor. Dynamic chunking or weighted ranges improve balance.

## Bottlenecks And Granularity

The benchmark is integer compute-bound with branch-heavy early exits. It has
almost no shared memory traffic beyond the final reduction. Chunk sizes should
contain many candidates so each worker amortizes task overhead and local count
reduction.

The algorithm is intentionally simple. Sieve-based prime counting would have a
very different memory profile and should not be compared as the same workload.

## References

- [Primality test overview](https://en.wikipedia.org/wiki/Primality_test)
- [Prime-counting function values](https://oeis.org/A006880)
- [Segmented sieve implementation background](https://en.wikipedia.org/wiki/Sieve_of_Eratosthenes#Segmented_sieve)
