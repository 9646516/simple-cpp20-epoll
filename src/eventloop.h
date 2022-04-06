#ifndef TIMER_EVENTLOOP_H
#define TIMER_EVENTLOOP_H

#include <utility>
#include <unordered_set>
#include <algorithm>
#include <queue>
#include <chrono>
#include <memory>
#include <chrono>
#include <optional>
#include "task.h"

inline const int SELECTOR_MAX_SELECT = 100;

struct EpollSelector {
    EpollSelector(const EpollSelector &) = delete;

    EpollSelector &operator=(const EpollSelector &) = delete;

    EpollSelector() : epoll_fd(epoll_create1(0)), size(0) {
        events.resize(SELECTOR_MAX_SELECT);
    }

    ~EpollSelector() { close(epoll_fd); }

    bool empty() const { return size == 0; }

    void insert_event(const int fd, std::coroutine_handle<void> handle) {
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.ptr = handle.address();
        int res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
        if (res == 0) {
            size++;
        }
    }

    void remove_event(const int fd) {
        epoll_event ev{};
        ev.events = EPOLLIN;
        int res = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &ev);
        if (res == 0) {
            size--;
        }
    }

    std::vector<std::coroutine_handle<void>> poll() {
        int num_ready = epoll_wait(epoll_fd, events.data(), SELECTOR_MAX_SELECT, -1);
        std::vector<std::coroutine_handle<void>> ret;
        for (int i = 0; i < num_ready; i++) {
            if (events[i].events & EPOLLIN) {
                ret.push_back(std::coroutine_handle<void>::from_address(events[i].data.ptr));
            }
        }
        return ret;
    }

    std::vector<epoll_event> events;
    int epoll_fd;
    int size;
};


class EventLoop {
public:
    EventLoop() {
        auto now = std::chrono::steady_clock::now();
    }

    void add_event(int fd, std::coroutine_handle<void> handle) {
        selector.insert_event(fd, handle);
    }

    void remove_event(int fd) {
        selector.remove_event(fd);
    }

    void run() {
        while (!selector.empty()) {
            auto event_lists = selector.poll();
            for (auto &&event: event_lists) {
                event.resume();
            }
        }
    }

    EventLoop(const EventLoop &) = delete;

    EventLoop &operator=(const EventLoop &) = delete;

    EpollSelector selector;


};


inline EventLoop *get_event_loop() {
    static EventLoop loop;
    return &loop;
}

template<typename T>
inline T block_on(Task<T> &&task) {
    auto loop = get_event_loop();
    task.handle.resume();
    loop->run();
    if constexpr(std::is_same_v<T, void>) {
        return;
    } else {
        return task.get();
    }
}


#endif
