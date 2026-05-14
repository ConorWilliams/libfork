---
icon: lucide/hash
---

# Primes

The primes benchmark counts prime numbers below `n` using trial division. The
primality test handles small divisors first and then checks candidates of the
form \(6k \pm 1\):

```cpp linenums="1"
for (std::int64_t i = 5; i * i <= n; i += 6) {
  if (n % i == 0 || n % (i + 2) == 0) {
    return false;
  }
}
```

The benchmark validates the configured sizes against known values of the
prime-counting function.

## Complexity

Testing one number \(x\) by trial division costs \(\mathcal{O}(\sqrt{x})\) in
the worst case. Counting all primes below \(n\) is therefore bounded by:

\[
T_1 = \mathcal{O}(n^{3/2})
\]

Each candidate number is independent, so the span is dominated by the most
expensive single primality test plus the final reduction:

\[
T_\infty = \mathcal{O}(\sqrt{n})
\]

## Scaling

Primes is a wide data-parallel benchmark with heterogeneous task costs. Larger
candidate numbers and prime candidates take longer to test than small composite
numbers.

Chunking contiguous ranges creates predictable memory access but uneven work.
Dynamic scheduling or smaller chunks improve load balance at the cost of more
task overhead.

## Benchmark sizes

The following problem sizes are available:

| Name | Count primes below | Expected count |
|------|---------------------|----------------|
| test | `100'000` | `9'592` |
| base | `10'000'000` | `664'579` |

## Results

TODO: results
