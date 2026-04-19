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
    WorkerPool::WorkerPool(uint8_t thread_count) : m_ThreadCount(thread_count)
    {
    }
    
    WorkerPool::~WorkerPool()
    {
        if (!m_Shutdown.load(std::memory_order_acquire))
        {
            stop();
        }
    }

    void WorkerPool::work()
    {
        m_Workers.reserve(m_ThreadCount);

        for (uint8_t worker_id = 0; worker_id < m_ThreadCount; worker_id++)
        {
            m_Workers.emplace_back([this, worker_id]() {
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
    
    void WorkerPool::stop()
    {
        m_Shutdown.store(true, std::memory_order_release);

        // Wait for all to complete
        for (auto& worker : m_Workers)
        {
            worker.join();
        }
    }

    void WorkerPool::worker_loop(int worker_id)
    {
        // HOT PATH! Ordering is not that important when shutting down...
        while (!m_Shutdown.load(std::memory_order_relaxed))
        {
            // TODO: Try pop local queue
            // TODO: Try steal from another, if none left
            // TODO: yield, if nothing
            // TODO: check for task deps
            // TODO: run task
            // TODO: enqueue dependencies
        }
    }
} // namespace tskr
