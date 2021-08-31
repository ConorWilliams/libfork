#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <thread>

#include "concurrentqueue.h"
#include "coop/meta.hpp"
#include "coop/sync_wait.hpp"
#include "coop/task.hpp"

class static_thread_pool {
  public:
    static void schedule(std::coroutine_handle<> handle) {
        static static_thread_pool pool;
        pool._queue.enqueue(handle);
    }

  private:
    static_thread_pool()
        : _thread([&](std::stop_token tok) {
              while (!tok.stop_requested()) {
                  std::coroutine_handle<> h;
                  if (_queue.try_dequeue(h)) {
                      std::cout << "Got one!\n";
                      h.resume();
                  }
              }
          }) {}

    moodycamel::ConcurrentQueue<std::coroutine_handle<>> _queue;

    std::jthread _thread;
};

riften::task<int, static_thread_pool> coro1() {
    std::cout << std::this_thread::get_id() << std::endl;
    co_return 1;
}

// riften::task<int, static_thread_pool> coro2() {
//     std::cout << "coro2\n";

//     auto a = coro1();

//     co_return 2;
// }

int main() {
    // static_thread_pool::schedule(nullptr);

    // auto task = coro3(thread_pool::executor());

    // auto a = static_thread_pool::fork(coro1());

    auto a = coro1();

    // riften::sync_wait(std::move(a));

    // std::cout << "Found: " << riften::sync_wait(std::move(a)) << std::endl;

    std::cout << "done\n";

    return 0;
}