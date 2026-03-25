#pragma once

#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <new>
#include <stdexcept>

static constexpr uint32_t MAX_TASKS = 65536;

using TaskFn = int (*)(int);

struct TaskMsg {
    TaskFn   func;
    int      arg;
    uint32_t task_id;
};

struct TaskResult {
    std::atomic<uint8_t> ready{0};
    int value;
};

struct PoolSHM {
    std::atomic<uint32_t> task_counter{0};
    TaskResult results[MAX_TASKS];
};

class Future {
public:
    Future(PoolSHM* shm, uint32_t id) : shm_(shm), id_(id) {}

    int Get() {
        auto& slot = shm_->results[id_ % MAX_TASKS];
        while (!slot.ready.load(std::memory_order_acquire)) {}
        int val = slot.value;
        slot.ready.store(0, std::memory_order_release);
        return val;
    }

private:
    PoolSHM* shm_;
    uint32_t id_;
};

class ProcessPool {
public:
    explicit ProcessPool(int n) : n_(n) {
        shm_ = static_cast<PoolSHM*>(
            mmap(nullptr, sizeof(PoolSHM), PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        if (shm_ == MAP_FAILED) throw std::runtime_error("mmap failed");
        new (shm_) PoolSHM{};

        if (pipe(work_pipe_) == -1) throw std::runtime_error("pipe failed");

        for (int i = 0; i < n_; ++i) {
            pid_t pid = fork();
            if (pid == -1) throw std::runtime_error("fork failed");
            if (pid == 0) {
                close(work_pipe_[1]);
                WorkerLoop();
                _exit(0);
            }
            workers_[i] = pid;
        }
        close(work_pipe_[0]);
    }

    ~ProcessPool() {
        TaskMsg quit{nullptr, 0, 0};
        for (int i = 0; i < n_; ++i)
            write(work_pipe_[1], &quit, sizeof(quit));
        for (int i = 0; i < n_; ++i)
            waitpid(workers_[i], nullptr, 0);
        close(work_pipe_[1]);
        munmap(shm_, sizeof(PoolSHM));
    }

    Future Submit(TaskFn func, int arg) {
        uint32_t id = shm_->task_counter.fetch_add(1, std::memory_order_relaxed);
        TaskMsg msg{func, arg, id};
        write(work_pipe_[1], &msg, sizeof(msg));
        return Future(shm_, id);
    }

private:
    void WorkerLoop() {
        TaskMsg msg;
        while (read(work_pipe_[0], &msg, sizeof(msg)) == sizeof(msg)) {
            if (msg.func == nullptr) break;
            int result = msg.func(msg.arg);
            shm_->results[msg.task_id % MAX_TASKS].value = result;
            shm_->results[msg.task_id % MAX_TASKS].ready.store(1, std::memory_order_release);
        }
    }

    static constexpr int kMaxWorkers = 64;
    int      n_;
    pid_t    workers_[kMaxWorkers];
    int      work_pipe_[2];
    PoolSHM* shm_;
};
