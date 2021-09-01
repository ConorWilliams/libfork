#include <chrono>
#include <iostream>
#include <stdexcept>
#include <stop_token>
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
    std::cout << "Thread: " << std::this_thread::get_id() << std::endl;
    co_return in;
}

hot_task<void, riften::Forkpool> test() {
    // std::cout << "Thread: " << std::this_thread::get_id() << std::endl;

    // std::cout << "then: " << co_await test(3) << " from " << std::this_thread::get_id() << std::endl;

    co_await test(0);

    co_return;
}

int main() {
    auto a = test();
    auto b = test();

    riften::sync_wait(a);

    riften::sync_wait(b);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "done\n";

    return 0;
}