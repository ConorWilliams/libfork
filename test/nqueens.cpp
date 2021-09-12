#include <iostream>
#include <vector>

#include "doctest/doctest.h"
#include "riften/task.hpp"

using namespace riften;

Task<int> nqueens(const int* a, int n, int d, int i) {
    std::vector<int> aa(d + 1);

    int j;

    for (j = 0; j < d; ++j) {
        aa[j] = a[j];

        int diff = a[j] - i;
        int dist = d - j;

        if (diff == 0 || dist == diff || dist + diff == 0) {
            co_return 0;
        }
    }

    if (d >= 0) aa[d] = i;
    if (++d == n) co_return 1;

    std::vector<Future<int>> res(n);

    for (i = 0; i < n; ++i) {
        res[i] = co_await fork(nqueens, aa.data(), n, d, i);
    }

    co_await tag_sync();

    int sum = 0;

    for (i = 0; i < n; ++i) {
        sum += *res[i];
    }

    co_return sum;
}

TEST_CASE("nqueens") {
    static int res[16] = {1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 2680, 14200, 73712, 365596, 2279184, 14772512};

    for (size_t i = 1; i < 15; i++) {  // 15
        CHECK(root(nqueens, nullptr, i, -1, 0) == res[i - 1]);
    }
}
