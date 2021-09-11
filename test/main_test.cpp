#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <syncstream>
#include <thread>
#include <vector>

#include "riften/forkpool.hpp"
#include "riften/meta.hpp"
#include "riften/task.hpp"

struct clock_tick {
    std::string name;
    typename std::chrono::high_resolution_clock::time_point start;
};

inline clock_tick tick(std::string const &name, bool print = false) {
    if (print) {
        std::cout << "Timing: " << name << '\n';
    }
    return {name, std::chrono::high_resolution_clock::now()};
}

template <typename... Args> int tock(clock_tick &x, Args &&...args) {
    using namespace std::chrono;

    auto const stop = high_resolution_clock::now();

    auto const time = duration_cast<microseconds>(stop - x.start).count();

    std::cout << x.name << ": " << time << "/us";

    (static_cast<void>(std::cout << ',' << ' ' << args), ...);

    std::cout << std::endl;

    return time;
}

using namespace riften;

// hot_Task<int, riften::Forkpool> test(int in) {
//     std::cout << "in a: " << std::this_thread::get_id() << std::endl;
//     co_return in;
// }

// struct threadX {
//     static void schedule(std::coroutine_handle<> handle) {
//         std::jthread([=] { handle(); }).detach();
//     }
// };

// using namespace riften;

// int volatile f;
// int volatile k;

// std::atomic_int count = 0;

// void do_work() { count.fetch_add(1, std::memory_order_release); }

// fork_Task<int> fib(int n) {
//     if (n < 0) {
//         throw std::invalid_argument("fib supports possitive numbers only");
//     }

//     switch (n) {
//         case 0:
//             do_work();
//             co_return 0;
//         case 1:
//             do_work();
//             co_return 1;
//         case 2:
//             do_work();
//             co_return 1;
//         default:
//             fork_Task<int> a = fib(n - 1);
//             fork_Task<int> b = fib(n - 2);
//             fork_Task<int> c = fib(n - 3);

//             co_return co_await a + co_await b + co_await c;
//     }
// }

// int linear(int n) {
//     if (n < 0) {
//         throw std::invalid_argument("fib supports possitive numbers only");
//     }

//     switch (n) {
//         case 0:
//             do_work();
//             return 0;
//         case 1:
//             do_work();
//             return 1;
//         case 2:
//             do_work();
//             return 1;
//         default:
//             auto a = linear(n - 1);
//             auto b = linear(n - 2);
//             auto c = linear(n - 3);

//             return a + b + c;
//     }
// }

// Task<int> fib2(int n) {
//     if (n < 0) {
//         throw std::invalid_argument("fib supports possitive numbers only");
//     }

//     switch (n) {
//         case 0:
//             do_work();
//             co_return 0;
//         case 1:
//             do_work();
//             co_return 1;
//         case 2:
//             do_work();
//             co_return 1;
//         default:
//             auto a = co_await fib2(n - 1).fork();
//             auto b = co_await fib2(n - 2).fork();
//             auto c = co_await fib2(n - 3).fork();

//             // co_await riften::sync();

//             co_return co_await a + co_await b + co_await c;
//     }
// }

// Task2<int> fib3(int n) {
//     if (n < 0) {
//         throw std::invalid_argument("fib supports possitive numbers only");
//     }

//     switch (n) {
//         case 0:
//             do_work();
//             co_return 0;
//         case 1:
//             do_work();
//             co_return 1;
//         case 2:
//             do_work();
//             co_return 1;
//         default:
//             auto a = co_await fib3(n - 1).fork();
//             auto b = co_await fib3(n - 2).fork();
//             auto c = co_await fib3(n - 3).fork();

//             co_await riften::sync();

//             co_return *a + *b + *c;
//     }
// }

#define FORK(...) co_await riften::fork(__VA_ARGS__);

Task<int> fib(int i) {
    if (i < 2) {
        co_return i;
    } else {
        Future a = co_await fork(fib, i - 1);
        Future b = co_await fork(fib, i - 2);

        co_await riften::sync();

        co_return *a + *b;
    }
}

Task<int> recur(int j) {
    if (j == 1) {
        co_return 1;
    }

    std::vector<Future<int>> fut;

    for (int i = 0; i < 1 + rand() % j; i++) {
        fut.emplace_back(co_await fork(recur, j - 1));
    }

    co_await riften::sync();

    int sum = 0;

    for (auto &&elem : fut) {
        sum += *elem;
    }

    co_return sum;
}

Task<> tmp2(int i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10 - i));
    std::cout << i << " nest\n";
    co_return;
}

// Task<void> tmp() {
//     Future f1 = co_await tmp2().fork();

//     co_await riften::sync();

//     std::cout << "nest\n";
//     co_return;
// }

Task<int> hello_world() {
    Future f1 = co_await tmp2(1).fork();
    Future f2 = co_await tmp2(2).fork();
    Future f3 = co_await tmp2(3).fork();
    Future f4 = co_await tmp2(4).fork();
    Future f5 = co_await tmp2(5).fork();

    co_await riften::sync();

    // std::cout << *f1 << " <- hello world\n";

    co_return 3;
}

int main() {
    /////

    // auto d = tick("super   ");
    // for (size_t i = 0; i < 1000; i++) {
    //     launch(fib(23));
    // }
    // auto tot = tock(d);

    // std::cout << "Got: " << tot / 1000 << std::endl;

    std::cout << launch(recur(3)) << " <- recur()\n";

    std::cout << "done\n";

    return 0;
}