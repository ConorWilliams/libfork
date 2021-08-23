#include <iostream>

#include "coop/meta.hpp"
#include "coop/task.hpp"

class static_thread_pool {
  public:
    static static_thread_pool& get() {
        static static_thread_pool pool;
        return pool;
    }

  private:
    static_thread_pool() {}
};

/*


fork<int> compute(int a){
    ...
}

auto a = compute(a)

*/

riften::task<int> coro() { co_return 3; }

int main() {
    std::cout << "done\n";

    auto a = coro();

    return 0;
}