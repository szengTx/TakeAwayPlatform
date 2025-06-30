#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include "task_queue.h"


namespace TakeAwayPlatform
{

    class ThreadPool 
    {
    public:
        explicit ThreadPool(size_t thread_count = std::thread::hardware_concurrency()) 
            : running(true) {
            workers.reserve(thread_count);
            for (size_t index = 0; index < thread_count; ++index) {
                workers.emplace_back([this] { worker_thread(); });
            }
        }

        ~ThreadPool() {
            running = false;
            condition.notify_all();
            for (auto& worker : workers) {
                if (worker.joinable()) worker.join();
            }
        }

        template<typename F>
        void enqueue(F&& task) {
            taskQueue.push(std::forward<F>(task));
            condition.notify_one();
        }

    private:
        void worker_thread() {
            while (running) {
                auto task = taskQueue.pop();
                if (task) task();
            }
        }

private:
        std::vector<std::thread> workers;
        TaskQueue taskQueue;
        std::atomic<bool> running;
        std::condition_variable condition;
        std::mutex mtx;
    };

}

