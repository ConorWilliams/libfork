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
#include "riften/sync_wait.hpp"
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

// hot_task<int, riften::Forkpool> test(int in) {
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

// fork_task<int> fib(int n) {
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
//             fork_task<int> a = fib(n - 1);
//             fork_task<int> b = fib(n - 2);
//             fork_task<int> c = fib(n - 3);

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

// task<int> fib2(int n) {
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

// task2<int> fib3(int n) {
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

task<int> hello_world() {
    std::cout << "hello world\n";
    co_return 3;
}

int main() {
    /////

    // int count = 20;

    // auto d = tick("super   ");
    // auto w = fib3(count).launch();
    // tock(d);

    // auto b = tick("linear  ");
    // auto y = linear(count);
    // tock(b);

    // auto a = tick("child   ");
    // auto x = riften::sync_wait(fib(count));
    // tock(a);

    // auto c = tick("continue");
    // auto z = launch(fib2(count));
    // tock(c);

    // std::cout << "Got: " << y << std::endl;
    // std::cout << "Got: " << x << std::endl;
    // std::cout << "Got: " << z << std::endl;
    // std::cout << "Got: " << w << std::endl;

    std::cout << "done\n";

    return 0;
}