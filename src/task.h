#ifndef TIMER_TASK_H
#define TIMER_TASK_H

#include <coroutine>
#include <cassert>
#include <variant>
#include <memory>
#include <optional>

template<typename T> concept is_awaiter = requires(T &&x) {
    { x.await_ready() };
    { x.await_resume() };
    { x.await_suspend(std::coroutine_handle<void>::from_address(nullptr)) };
};
template<class T>
struct Task;

template<typename T>
struct promise_type_base {
    std::coroutine_handle<void> fa;

    promise_type_base() : fa(nullptr) {}


    std::suspend_always initial_suspend() { return {}; }

    void unhandled_exception() {}

    auto final_suspend() noexcept {
        struct TaskAwaiter {
            std::coroutine_handle<void> fa;

            bool await_ready() noexcept { return false; }

            std::coroutine_handle<void> await_suspend(std::coroutine_handle<>) noexcept {
                return fa ? fa : std::noop_coroutine();
            }

            void await_resume() noexcept {}
        };
        return TaskAwaiter{fa};
    }

    template<class T2>
    requires is_awaiter<T2>
    auto await_transform(const T2 &awaiter) { return std::move(awaiter); }

    template<class T2>
    auto await_transform(const Task<T2> &task) {
        struct TaskAwaiter {
            std::coroutine_handle<typename Task<T2>::promise_type> handle;

            bool await_ready() {
                return handle.promise().is_done();
            }

            void await_suspend(std::coroutine_handle<> h) {
                handle.promise().fa = static_cast<std::coroutine_handle<void>>(h);
            }

            T2 await_resume() {
                if constexpr(!std::is_same_v<T2, void>) {
                    return std::move(*(handle.promise().result));
                }
            }
        };
        if (!task.handle.done()) {
            task.handle.resume();
        }
        return TaskAwaiter{task.handle};
    }
};


template<typename T>
struct Task {
    struct promise_type : public promise_type_base<T> {
        std::optional<T> result;

        bool is_done() const { return result.has_value(); }

        promise_type() : result(std::nullopt) {}

        void return_value(T val) { result = std::move(val); }

        Task<T> get_return_object() {
            return Task(std::coroutine_handle<promise_type>::from_promise(*this));
        }
    };

    T get() { return this->handle.promise().result.value(); }

    ~Task() { handle.destroy(); }

    Task(const Task &) = delete;

    Task &operator=(const Task &) = delete;

    explicit Task(const std::coroutine_handle<promise_type> _handle) {
        this->handle = _handle;
    }

    Task(const Task &&rhs) noexcept {
        this->handle.destroy();
        this->handle = rhs.handle;
    }

    std::coroutine_handle<promise_type> handle;
};

template<>
struct Task<void> {
    struct promise_type : public promise_type_base<void> {
        bool is_done() { return done; }

        Task get_return_object() {
            return Task(std::coroutine_handle<promise_type>::from_promise(*this));
        }

        bool done;

        promise_type() : done(false) {}

        void return_void() { done = true; }
    };

    ~Task() { handle.destroy(); }

    Task(const Task &) = delete;

    Task &operator=(const Task &) = delete;

    explicit Task(const std::coroutine_handle<promise_type> _handle) {
        this->handle = _handle;
    }

    Task(const Task &&rhs) noexcept {
        this->handle.destroy();
        this->handle = rhs.handle;
    }

    std::coroutine_handle<promise_type> handle;
};


#endif
