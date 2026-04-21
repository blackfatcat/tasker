#pragma once

#include <vector>
#include <atomic>
#include <thread>
#include <cstdint>

#include "task.hpp"
#include "queue.hpp"
#include "mpmc_queue.hpp"

namespace tskr
{
    class WorkerPool
    {
    private:
        std::vector<std::thread> m_Workers;

        std::vector<std::unique_ptr<WorkStealingDeque<TaskNode>>> m_LocalQueues;
        rigtorp::MPMCQueue<std::shared_ptr<TaskNode>> m_GlobalQueue;

        size_t m_WorkerCap;
        static thread_local int s_WorkerId;

        std::atomic<size_t> m_TasksRemaining{ 0 };
        std::atomic<bool> m_Shutdown{ false };
        uint8_t m_ThreadCount;
    public:
        // TODO: Add affinity mask
        WorkerPool(uint8_t thread_count = std::thread::hardware_concurrency(), size_t per_worker_cap = 1024);
        ~WorkerPool();

        /// @brief Add a task to be executed
        /// @brief Note: if a worker thread adds it, will be added directly to its own worker queue <br>
        /// @brief Note: if a non-worker thread adds it (the main thread), will be added to the global queue for any* thread to grab
        void enqueue(std::shared_ptr<TaskNode> task);

        /// @brief Start the worker loops
        void work();
        
        /// @brief Busy-wait for all tasks currently being executed to finish 
        void wait_for_all();

        /// @brief Checks if there are any tasks still running
        /// @return `true` if no tasks remain, false otherwise
        bool try_wait_for_all();

        /// @brief Stop the worker loops and join all worker threads
        void stop();

        /// @brief Atomically set the number of tasks remaining
        void set_task_count(size_t new_count);

    private:
        void worker_loop(int worker_id);
    };
    
} // namespace tskr
