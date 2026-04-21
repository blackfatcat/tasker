#include "worker_pool.hpp"

#ifdef TASKER_WINDOWS

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif // TSKR_WINDOWS

#ifdef TASKER_LINUX
#include <pthread.h>
#include <sched.h>
#endif // TSKR_LINUX

namespace tskr
{
    thread_local int WorkerPool::s_WorkerId = -1;

    WorkerPool::WorkerPool(uint8_t thread_count, size_t per_worker_cap) 
        : 
        m_ThreadCount(thread_count),
        m_GlobalQueue{per_worker_cap * (size_t)thread_count},
        m_WorkerCap(per_worker_cap)
    {
        m_Workers.reserve(m_ThreadCount);
        m_LocalQueues.reserve(m_ThreadCount);

        for (uint8_t worker_id = 0; worker_id < m_ThreadCount; worker_id++)
        {
            // Create worker queues
            m_LocalQueues.emplace_back(std::make_unique<WorkStealingDeque<TaskNode>>(m_WorkerCap));
        }
    }
    
    WorkerPool::~WorkerPool()
    {
        stop();
    }

    void WorkerPool::enqueue(TaskNode* task)
    {
        int id = s_WorkerId;

        m_TasksRemaining.fetch_add(1, std::memory_order_release);

        if (id >= 0 && id < m_ThreadCount)
        {
            // Worker thread called -> local push
            if (!m_LocalQueues[id]->try_push(task))
                m_GlobalQueue.push(task); // Fall back to global if local is empty
        }
        else
        {
            m_GlobalQueue.push(task);
        }
    }

    void WorkerPool::work()
    {
        for (uint8_t worker_id = 0; worker_id < m_ThreadCount; worker_id++)
        {
            m_Workers.emplace_back([this, worker_id]() {
                s_WorkerId = worker_id;

                // Set affinity for each tread created. !! CURRENTLY each thread will like just one core (the one its created on)
                // TODO: use affinity mask for multi-core affinities
#ifdef TASKER_WINDOWS
                SetThreadAffinityMask(GetCurrentThread(), 1ull << worker_id);
#endif

#ifdef TASKER_LINUX
                // (Linux only!) Create a CPU set with reserved space for the number of threads
                cpu_set_t* cpu_set;
                cpu_set = CPU_ALLOC(m_ThreadCount);

                size_t size;
                size = CPU_ALLOC_SIZE(m_ThreadCount);

                CPU_ZERO_S(size, cpu_set);

                CPU_SET_S(worker_id, size, cpu_set);
                pthread_setaffinity_np(pthread_self(), size, cpu_set);

                CPU_ZERO_S(size, cpu_set);
                CPU_FREE(cpu_set);
#endif
                // Aaaaand... we are offf!!
                worker_loop(worker_id);
            });
        }

#ifdef TASKER_LINUX
#endif
    }

    void WorkerPool::wait_for_all()
    {
        while (m_TasksRemaining.load(std::memory_order_acquire) > 0) {}
    }

    bool WorkerPool::try_wait_for_all()
    {
        if (m_TasksRemaining.load(std::memory_order_acquire) > 0)
            return false;
        
        return true;
    }
    
    void WorkerPool::stop()
    {
        bool shutdown = false;
        if (m_Shutdown.compare_exchange_strong(shutdown, true, std::memory_order_acq_rel))
        
        // Wait for all to complete
        for (auto& worker : m_Workers)
        {
            worker.join();
        }
    }

    void WorkerPool::worker_loop(int worker_id)
    {
        auto& local_queue = m_LocalQueues[worker_id];

        // HOT PATH! Ordering is not that important when shutting down...
        while (!m_Shutdown.load(std::memory_order_relaxed))
        {
            TaskNode* task_node = nullptr;

            // Local queue empty?
            if (!local_queue->try_pop(task_node))
            {
                // Try stealing from other workers
                for (size_t i = 0; i < m_ThreadCount && !task_node; i++)
                {
                    if (i == worker_id) continue;
                    m_LocalQueues[i]->try_steal(task_node);
                }
            }

            // Still no task? 
            if (!task_node)
                m_GlobalQueue.try_pop(task_node);

            if (!task_node)
            {
                // No work to do...
                std::this_thread::yield();
                continue;
            }

            task_node->task->fun(task_node->task->payload);

            m_TasksRemaining.fetch_sub(1, std::memory_order_release);
            
            // TODO: check for task deps
            // TODO: run task
            // TODO: enqueue dependencies
        }
    }
} // namespace tskr
