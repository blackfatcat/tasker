#pragma once

#include <cassert>
#include <atomic>
#include <vector>
#include <memory>

namespace tskr
{
    // Push and Pop to and from the back. Steal from the front. Chase-Lev style work-stealing double ended queue. Efficient for thread local queues.
    // The algorithm holds only if push and pop are performed by the same thread.
    // Other threads can only call steal on the queue.
    template<typename T>
    class WorkStealingDeque
    {
    private:
        size_t m_Cap; 
        std::vector<std::unique_ptr<std::atomic<T*>>> m_TaskBuf;
        std::atomic<size_t> m_Top;
        std::atomic<size_t> m_Bottom;
    public:
        WorkStealingDeque(size_t cap = 1024)
            : m_Cap(cap),
            m_Top(0),
            m_Bottom(0)
        {
            m_TaskBuf.reserve(m_Cap);
        }
        ~WorkStealingDeque()
        {
            m_TaskBuf.clear();
        }

        WorkStealingDeque(const WorkStealingDeque&) = delete;
        WorkStealingDeque& operator=(const WorkStealingDeque&) = delete;
        WorkStealingDeque(WorkStealingDeque&&) = delete;
        WorkStealingDeque& operator=(WorkStealingDeque&&) = delete;

        /// @brief
        /// Pushes a task to the back of the queue, doubling its size if no space is left
        /// @param task Task to be pushed
        void push(T* task)
        {
            size_t bottom = m_Bottom.load(std::memory_order_relaxed);
            size_t top = m_Top.load(std::memory_order_relaxed);

            // Full?
            if (bottom - top >= m_Cap)
            {
                m_Cap *= 2;
                m_TaskBuf.reserve(m_Cap);
            }

            m_TaskBuf[bottom & (m_Cap - 1)]->store(task, std::memory_order_relaxed);
            m_Bottom.store(bottom + 1, std::memory_order_release);
        }

        /// @brief
        /// Tries to push a task to the back of the queue, without increasing the size if no space is left
        /// @param task Task to be pushed
        /// @return `true` if task was successfully pushed, `false` otherwise
        bool try_push(T* task)
        {
            size_t bottom = m_Bottom.load(std::memory_order_relaxed);
            size_t top = m_Top.load(std::memory_order_relaxed);

            // Full?
            if (bottom - top >= m_Cap)
                return false;

            m_TaskBuf[bottom & (m_Cap - 1)]->store(task, std::memory_order_relaxed);
            m_Bottom.store(bottom + 1, std::memory_order_release);

            return true;
        }

        /// @brief
        /// Tries to retrieve a task from the front of the queue
        /// @param task Reference to a task that will be filled up, if the operation succeeds
        /// @return `true` if task was successfully retrieved, `false` otherwise
        bool try_pop(T* task)
        {
            size_t bottom = m_Bottom.load(std::memory_order_relaxed);
            size_t top = m_Top.load(std::memory_order_relaxed);

            // Nothing left
            if (bottom == top)
                return false;

            --bottom;
            m_Bottom.store(bottom); // Ceq_cst enforeces total order with thieves's load

            T* t = m_TaskBuf[bottom & (m_Cap - 1)]->load(std::memory_order_relaxed);

            top = m_Top.load(); // Ceq_cst enforeces total order with thieves's CAS

            if (bottom > top)
            {
                task = t;
                return true;
            }

            // Last task?
            if (bottom == top)
            {
                // Make sure noone has stolen it and cannot steal it
                if (m_Top.compare_exchange_strong(top, top + 1, std::memory_order_relaxed))
                {
                    m_Bottom.store(top + 1, std::memory_order_relaxed);
                    task = t;
                    return true;
                }

                m_Bottom.store(top, std::memory_order_relaxed);
                return false;
            }

            assert(bottom == top - 1);
            m_Bottom.store(top, std::memory_order_relaxed);
            return false;
        }

        /// @brief
        /// Tries to retrieve a task from the front of the queue
        /// @param task Reference to a task that will be filled up, if the operation succeeds
        /// @return `true` if task was successfully retrieved, `false` otherwise
        bool try_steal(T* task)
        {
            size_t top = m_Top.load(std::memory_order_relaxed);
            size_t bottom = m_Bottom.load(); // Ceq_cst enforces total order with the thread stolen from

            // Empty?
            if (bottom - top <= 0)
                return false;

            T* t = m_TaskBuf[top & (m_Cap - 1)]->load(std::memory_order_relaxed);

            // Make sure it was not consumed in the mean time
            if (m_Top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
            {
                task = t;
                return true;
            }

            return false;
        }
    };
} // namespace tskr
