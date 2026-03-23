#pragma once

#include <coroutine>
#include <exception>
#include <optional>

template <typename T>
class Generator {
public:
    struct promise_type {
        std::optional<T> value;

        Generator get_return_object() {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(T val) {
            value = std::move(val);
            return {};
        }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    explicit Generator(std::coroutine_handle<promise_type> h) : handle_(h) {}
    ~Generator() { if (handle_) handle_.destroy(); }

    Generator(const Generator&) = delete;
    Generator(Generator&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

    bool Next() {
        handle_.resume();
        return !handle_.done();
    }

    T Value() const { return *handle_.promise().value; }

private:
    std::coroutine_handle<promise_type> handle_;
};
