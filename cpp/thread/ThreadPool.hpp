/*
* ThreadPool Class
* is thread pool that uses a priority queue to manage tasks.
* It created for easy multi-threading environment use.
* By default, when a Task is pushed, an idle thread is automatically assigned and executed.
* If necessary, you can be stopped or waited for the task to be completed.
* Task should be function, the return value cannot be gotten
* 
* ref:
* https://modoocode.com/285
*/
#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <functional>

class ThreadPool {
public:
    using Work = std::function<void()>;
    using Task = std::pair<int, Work>;  // pair: {priority, work_function}

    static constexpr int DEFAULT_PRIORITY = 0;

    // Constructor
    ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) {}
    explicit ThreadPool(size_t threadCount) {
        size_t hw = std::thread::hardware_concurrency();
        threadCount_ = (threadCount == 0 || threadCount > hw) ? hw : threadCount;
        threadCount_ = std::max(threadCount_, 1llu);

        threads_.resize(threadCount_);
        stopFlag_.store(false, std::memory_order_release);
        pauseFlag_.store(false, std::memory_order_release);
        runningTasks_ = 0;

        // Start worker threads
        for (size_t i = 0; i < threadCount_; ++i) {
            threads_[i] = std::thread(&ThreadPool::workerLoop, this);
        }
    }
    // Destructor
    ~ThreadPool() { shutdown(); }

    // -------------------------------- Methods --------------------------------

    // Enqueue the task
    void enqueue(Work work) {
        enqueue(std::move(work), DEFAULT_PRIORITY);
    }

    // Enqueue the task with specified priority, higher int means higher priority
    // example:
    // enqueue(foo, 1);
    void enqueue(Work work, int priority) {
        {   // Lock
            std::unique_lock<std::mutex> lock(queueMutex_);
            taskQueue_.emplace(priority, std::move(work));
        }   // Unlock

        // Notify one waiting worker
        if (!pauseFlag_.load(std::memory_order_acquire)) {
            queueCV_.notify_one();
        }
    }

    // Wait until the queue is empty and all running tasks are complete
    void wait() {
        std::unique_lock<std::mutex> lock(queueMutex_);

        waitCV_.wait(lock, [this]() {
            return taskQueue_.empty() && (runningTasks_.load(std::memory_order_acquire) == 0);
            });
    }

    // Pause the working of tasks
    void pause() {
        pauseFlag_.store(true, std::memory_order_release);
    }

    // Resume the working of tasks
    void resume() {
        pauseFlag_.store(false, std::memory_order_release);
        queueCV_.notify_all();
    }

    // Clear all waiting tasks from the queue
    void clearQueue() {
        std::unique_lock<std::mutex> lock(queueMutex_);
        std::priority_queue<Task, std::vector<Task>, TaskCompare> empty_queue;
        taskQueue_.swap(empty_queue);

        // If no tasks are running, notify wait calls that the pool is idle
        if (runningTasks_.load(std::memory_order_acquire) == 0) {
            waitCV_.notify_all();
        }
    }

    // Shut down the thread pool and destroy worker threads
    // New thread pool must be used to enable thread again after calling this method
    void shutdown() {
        {   // Lock
            std::unique_lock<std::mutex> lock(queueMutex_);
            stopFlag_.store(true, std::memory_order_release);
            pauseFlag_.store(false, std::memory_order_release);
        }   // Unlock

        queueCV_.notify_all();
        waitCV_.notify_all();

        for (auto& t : threads_) {
            if (t.joinable()) t.join();
        }
    }

    // ----------------------------- Status & Stats -----------------------------
    
    // Return the number of waiting task in queue
    size_t getQueueSize() const {
        std::unique_lock<std::mutex> lock(queueMutex_);
        return taskQueue_.size();
    }

    // Return the number of activate threads
    size_t getThreadCount() const { return threadCount_; }

    // Return the number of running tasks
    int getRunningTasks() const { return runningTasks_.load(std::memory_order_acquire); }

    // Return the status of pause flag
    // true: paused
    // false: working
    bool isPaused() const { return pauseFlag_.load(std::memory_order_acquire); }

    // Return the status of stop flag
    // true: stopped
    // flase: active
    bool isStopped() const {
        return stopFlag_.load(std::memory_order_acquire);
    }

private:

    // Comparator for the priority queue, compares only the first element(int priority)
    struct TaskCompare {
        bool operator()(const Task& lhs, const Task& rhs) const {
            return lhs.first < rhs.first;
        }
    };

    // Main loop for each worker thread
    void workerLoop() {
        while (true) {
            Work work;

            {   // Lock
                std::unique_lock<std::mutex> lock(queueMutex_);

                // Wait condition
                queueCV_.wait(lock, [&]() {
                    return stopFlag_.load(std::memory_order_acquire) || (!pauseFlag_.load(std::memory_order_acquire) && !taskQueue_.empty());
                    });

                // Exit condition
                if (stopFlag_.load(std::memory_order_acquire) && taskQueue_.empty()) { return; }

                // Pause condition
                if (pauseFlag_.load(std::memory_order_acquire)) { continue; }

                if (!taskQueue_.empty()) {
                    //work = std::move(taskQueue_.top().second);  // std::move const warning, taskQueue_.top().second is const Work& type
                    work = taskQueue_.top().second;
                    taskQueue_.pop();
                }
                else { continue; }
            }   // Unlock

            // Execute the task
            runningTasks_.fetch_add(1, std::memory_order_relaxed);
            work();

            {   // Lock
                std::unique_lock<std::mutex> lock(queueMutex_);

                // Decrement running task count
                // If this was the last running task, check if we need to notify wait
                if (runningTasks_.fetch_sub(1, std::memory_order_release) == 1) {
                    if (taskQueue_.empty()) {
                        waitCV_.notify_all();  // Notify waiting threads (pool is idle)
                    }
                }
            }   // Unlock
        }   // End of while
    }

private:
    std::atomic<bool> stopFlag_{ false };       // Signals workers to terminate permanently
    std::atomic<bool> pauseFlag_{ false };      // Signals workers to temporarily stop consuming tasks
    std::atomic<int> runningTasks_{ 0 };        // Counter for currently executing tasks

    size_t threadCount_ = 0;                    // Target number of worker threads
    std::vector<std::thread> threads_;          // Container for worker threads

    // Priority queue of tasks, sort by priority
    std::priority_queue<Task, std::vector<Task>, TaskCompare> taskQueue_;

    mutable std::mutex queueMutex_;             // Mutex to protect the task queue
    std::condition_variable queueCV_;           // CV to signal workers when a new task arrives (or shutdown)
    std::condition_variable waitCV_;            // CV to signal threads waiting on wait() when the pool becomes idle
};

/* Usage Example

void simple_task(int id, double val) {
    // some magic
}

int main() {
    // Thread count will be automatically clamped to hardware_concurrency()
    // You may call std::thread::hardware_concurrency() to check your system's maximum thread count.
    ThreadPool pool(4);

    // Enqueue task default priority
    // The Task signature is function<void()>
    // To pass arguments to a task, wrap it with a lambda or std::bind
    // auto bound_task = [i, j](){ simple_task(i, j); };    // basic way to parameter capture
    // auto bound_task = std::bind(simple_task, i, j);      // or you can also use std::bind 
    pool.enqueue([]{ simple_task(1, 2.5); });               // or use lambda (recommend)

    // You can set task priority
    pool.enqueue([]{ simple_task(3, 1.2); }, -5);           // Low priority (executed later)
    pool.enqueue([]{ simple_task(7, 8.3); }, 8);            // High priority (executed earlier)

    // Wait for all tasks to complete before proceeding
    pool.wait();
}

*/
