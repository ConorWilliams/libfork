#include "doctest/doctest.h"
#include "riften/task.hpp"

using namespace riften;

int fib_fast(int n) {
    if (n < 2) {
        return n;
    } else {
        int i = 2, x = 0, y = 0, z = 1;
        do {
            x = y;
            y = z;
            z = x + y;
        } while (i++ < n);

        return z;
    }
}

Task<int> fib(int n) {
    if (n < 2) {
        co_return n;
    } else {
        Future a = co_await fork(fib, n - 1);
        Future b = co_await fork(fib, n - 2);

        co_await tag_sync();

        co_return *a + *b;
    }
}

TEST_CASE("fib") {
    for (size_t i = 0; i < 38; i++) {  // 38
        CHECK(root(fib, i) == fib_fast(i));
    }
}
