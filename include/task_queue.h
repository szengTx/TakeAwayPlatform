#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>


namespace TakeAwayPlatform
{

    class TaskQueue 
    {
    public:
        using Task = std::function<void()>;

        void push(Task task) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                tasks.push(std::move(task));
            }
            condition.notify_one();
        }

        Task pop() {
            std::unique_lock<std::mutex> lock(mtx);
            condition.wait(lock, [this] { return !tasks.empty(); });
            
            Task task = std::move(tasks.front());
            tasks.pop();
            return task;
        }

        bool empty() const {
            std::lock_guard<std::mutex> lock(mtx);
            return tasks.empty();
        }

    private:
        std::queue<Task> tasks;
        mutable std::mutex mtx;
        std::condition_variable condition;
    };

} 