#ifndef TIMER_TIMER_H
#define TIMER_TIMER_H

#include <iostream>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <chrono>
#include <coroutine>
#include "eventloop.h"
#include "task.h"


struct TimerAwaiter {
    TimerAwaiter(const TimerAwaiter &) = delete;

    TimerAwaiter(const TimerAwaiter &&rhs) noexcept {
        this->fd = rhs.fd;
    }

    TimerAwaiter &operator=(const TimerAwaiter &) = delete;

    explicit TimerAwaiter(int _fd) : fd(_fd) {}

    bool await_ready() const noexcept { return fd > 0; }

    void await_resume() noexcept {
        static uint64_t exp;
        read(fd, &exp, sizeof(uint64_t));
        close(fd);
        fd = -1;
        auto loop = get_event_loop();
        loop->remove_event(fd);
    }

    template<typename T>
    void await_suspend(std::coroutine_handle<T> caller) const noexcept {
        auto loop = get_event_loop();
        loop->add_event(fd, caller);
    }

    int fd;
};

struct Timer {
    static TimerAwaiter sleep_for(const int sec, const int nsec) {
        int fd = timerfd_create(CLOCK_REALTIME, 0);
        itimerspec new_value{};
        timespec now{};
        clock_gettime(CLOCK_REALTIME, &now);

        new_value.it_value.tv_sec = now.tv_sec + sec;
        new_value.it_value.tv_nsec = now.tv_nsec + nsec;

        new_value.it_interval.tv_sec = 0;
        new_value.it_interval.tv_nsec = 0;

        timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_value, nullptr);
        return TimerAwaiter{fd};
    }
};

#endif