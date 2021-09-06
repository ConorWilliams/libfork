#include <chrono>
#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <syncstream>
#include <thread>
#include <vector>

#include "forkpool/forkpool.hpp"
#include "forkpool/hot_task.hpp"
#include "forkpool/meta.hpp"
#include "forkpool/sync_wait.hpp"

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

struct spawn_thread {
    static void schedule(std::coroutine_handle<> handle) {
        std::jthread thread([handle] { handle.resume(); });
        thread.detach();
    }
};

using namespace riften;

hot_task<int, riften::Forkpool> test(int in) {
    std::cout << "in a: " << std::this_thread::get_id() << std::endl;
    co_return in;
}

struct threadX {
    static void schedule(std::coroutine_handle<> handle) {
        std::jthread([=] { handle(); }).detach();
    }
};

int volatile f;
int volatile k;

hot_task<int, riften::Forkpool> fib(int n) {
    if (n <= 0) {
        throw std::invalid_argument("fib supports possitive numbers only");
    }

    switch (n) {
        case 1:
            co_return 1;
        case 2:
            co_return 1;
        default:
            auto a = fib(n - 1);
            auto b = fib(n - 2);

            co_return co_await a + co_await b;
    }
}

riften::hot_task<int, riften::Forkpool> random(int n) {
    if (n <= 0) {
        for (size_t i = 0; i < 1000000; i++) {
            f = k;
        }
        co_return 1;
    } else {
        std::vector<riften::hot_task<int, riften::Forkpool>> jobs;

        int count = rand() % 10;

        assert(count < 10 && count > 0);

        for (int i = 0; i < count; i++) {
            jobs.emplace_back(random(n - 1));
        }

        int sum = 0;

        for (auto &&elem : jobs) {
            sum += co_await elem;
        }

        co_return sum;
    }
}

int main() {
    auto a = tick("fib");

    std::cout << "Got: " << riften::sync_wait(fib(38)) << std::endl;

    tock(a);

    auto b = tick("strict");

    // std::cout << "Got: " << riften::sync_wait(fib_strict(38)) << std::endl;

    tock(b);

    std::cout << "done\n";

    return 0;
}