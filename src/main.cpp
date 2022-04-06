#include <iostream>
#include <chrono>
#include "timer.h"
#include "task.h"
#include "eventloop.h"

template<class T>
Task<T> gao_typed() {
    for (int i = 0; i < 2; i++) {
        co_await Timer::sleep_for(1, 0);
        std::cout << "gao_typed " << i << std::endl;
    }
    co_return T(1);
}

Task<void> gao_void() {
    for (int i = 0; i < 2; i++) {
        co_await Timer::sleep_for(1, 0);
        std::cout << "gao_void " << i << std::endl;
    }
    co_return;
}

Task<int> gao() {
    int res1 = co_await gao_typed<int>();
    bool res2 = co_await gao_typed<bool>();
    uint16_t res3 = co_await gao_typed<uint16_t>();
    std::cout << res1 << ' ' << res2 << ' ' << res3 << std::endl;
    co_await gao_void();

    std::cout << "begin duplicate await" << std::endl;
    auto task1 = gao_void();
    co_await task1;
    co_await task1;

    auto task2 = gao_typed<uint8_t>();
    co_await task2;
    co_await task2;

    co_return 5;
}

int main() {
    auto task = gao();
    auto s = std::chrono::steady_clock::now();
    auto res = block_on(std::move(task));
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - s).count() << std::endl;
    std::cout << "ret " << res << std::endl;
    return 0;
}