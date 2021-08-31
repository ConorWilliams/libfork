/*  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)
To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.
See <http://creativecommons.org/publicdomain/zero/1.0/>. */

#include <stdint.h>

#include <array>

namespace riften::detail {

static inline uint64_t rotl(const uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }

// Seeded from www.random.org
static thread_local uint64_t s[4] = {
    0x0371e0ea0605dad0,
    0x073850f2da03e450,
    0x0d73b0d4a207c300,
    0x061140737f0d2070,
};

/* This is xoshiro256++ 1.0, one of our all-purpose, rock-solid generators.
   It has excellent (sub-ns) speed, a state (256 bits) that is large
   enough for any parallel application, and it passes all tests we are
   aware of.
   For generating just floating-point numbers, xoshiro256+ is even faster.
   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */
inline uint64_t xrand(void) {
    const uint64_t result = rotl(s[0] + s[3], 23) + s[0];

    const uint64_t t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;

    s[3] = rotl(s[3], 45);

    return result;
}

/* This is the jump function for the generator. It is equivalent
   to 2^128 calls to xrand(); it can be used to generate 2^128
   non-overlapping subsequences for parallel computations. */
inline void jump(void) {
    //
    static constexpr std::array<uint64_t, 4> JUMP = {
        0x180ec6d33cfd0aba,
        0xd5a61266f0c9392c,
        0xa9582618e03fc9aa,
        0x39abdc4529b1661c,
    };

    uint64_t s0 = 0;
    uint64_t s1 = 0;
    uint64_t s2 = 0;
    uint64_t s3 = 0;

    for (auto&& j : JUMP)
        for (int b = 0; b < 64; b++) {
            if (j & UINT64_C(1) << b) {
                s0 ^= s[0];
                s1 ^= s[1];
                s2 ^= s[2];
                s3 ^= s[3];
            }
            xrand();
        }

    s[0] = s0;
    s[1] = s1;
    s[2] = s2;
    s[3] = s3;
}

/* This is the long-jump function for the generator. It is equivalent to
   2^192 calls to xrand(); it can be used to generate 2^64 starting points,
   from each of which jump() will generate 2^64 non-overlapping
   subsequences for parallel distributed computations. */
inline void long_jump(void) {
    //
    static constexpr std::array<uint64_t, 4> LONG_JUMP = {
        0x76e15d3efefdcbbf,
        0xc5004e441c522fb3,
        0x77710069854ee241,
        0x39109bb02acbe635,
    };

    uint64_t s0 = 0;
    uint64_t s1 = 0;
    uint64_t s2 = 0;
    uint64_t s3 = 0;

    for (auto&& j : LONG_JUMP)
        for (int b = 0; b < 64; b++) {
            if (j & UINT64_C(1) << b) {
                s0 ^= s[0];
                s1 ^= s[1];
                s2 ^= s[2];
                s3 ^= s[3];
            }
            xrand();
        }

    s[0] = s0;
    s[1] = s1;
    s[2] = s2;
    s[3] = s3;
}

}  // namespace riften::detail