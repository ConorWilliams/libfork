#include <chrono>
#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>

#include "forkpool/forkpool.hpp"
#include "forkpool/hot_task.hpp"
#include "forkpool/meta.hpp"
#include "forkpool/sync_wait.hpp"

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

hot_task<void, riften::Forkpool> test() {
    auto thread = std::this_thread::get_id();

    std::cout << "in b: " << thread << std::endl;

    // co_await test(0);

    std::cout << "post b: " << thread << std::endl;

    co_return;
}

int main() {
    // while (true) {
    auto a = test();
    riften::sync_wait(a);
    // }

    std::cout << "done\n";

    return 0;
}