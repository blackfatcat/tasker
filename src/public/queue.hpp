#pragma once

#include <atomic>
#include <vector>
#include <memory>

namespace tskr
{
    // Chase-Lev style work-stealing double ended queue.
    // Push and Pop to and from the back. Steal from the front. Efficient for thread local queues.
    // The algorithm holds only if push and pop are performed by the same thread.
    // Other threads can only call steal on the queue.
    template<typename T>
    class WorkStealingDeque
    {
    private:
        size_t m_Cap;

        std::atomic<T*>* m_TaskBuf;
        std::atomic<size_t> m_Top;
        std::atomic<size_t> m_Bottom;
    public:
        WorkStealingDeque(size_t cap = 1024)
            : m_Cap(cap),
            m_Top(0),
            m_Bottom(0)
        {
            m_TaskBuf = new std::atomic<T*>[m_Cap];

            for (size_t i = 0; i < m_Cap; i++)
            {
                m_TaskBuf[i] = nullptr;
            }
        }
        ~WorkStealingDeque()
        {
            delete[] m_TaskBuf;
        }

        // TODO: delete copy and move constructors and operator= when replaced with no-grow vec

        WorkStealingDeque(const WorkStealingDeque& other)
        {
            copy_from_other(other);
        }

        WorkStealingDeque& operator=(const WorkStealingDeque& other)
        {
            copy_from_other(other);

            return *this;
        }

        WorkStealingDeque(const WorkStealingDeque&& other)
        {
            copy_from_other(other);
        }

        WorkStealingDeque& operator=(const WorkStealingDeque&& other)
        {
            copy_from_other(other);

            return *this;
        }

        void copy_from_other(const WorkStealingDeque& other)
        {
            this->m_Top.exchange(other.m_Top.load(std::memory_order_relaxed), std::memory_order_relaxed);
            this->m_Bottom.exchange(other.m_Bottom.load(std::memory_order_relaxed), std::memory_order_relaxed);
            this->m_Cap = other.m_Cap;
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

            m_TaskBuf[bottom & (m_Cap - 1)].store(task, std::memory_order_relaxed);
            m_Bottom.store(bottom + 1, std::memory_order_release);

            return true;
        }

        /// @brief
        /// Tries to retrieve a task from the front of the queue
        /// @param task Reference to a task that will be filled up, if the operation succeeds
        /// @return `true` if task was successfully retrieved, `false` otherwise
        T* try_pop()
        {
            size_t bottom = m_Bottom.load(std::memory_order_relaxed);

            bottom = bottom > 0 ? bottom - 1 : 0;

            m_Bottom.store(bottom); // Ceq_cst enforeces total order with thieves's load

            size_t top = m_Top.load(); // Ceq_cst enforeces total order with thieves's CAS

            // Has anaything?
            if (top <= bottom)
            {
                // Last element?
                if (top == bottom)
                {
                    if (!m_Top.compare_exchange_strong(top, top + 1, std::memory_order_relaxed))
                    {
                        m_Bottom.store(bottom + 1, std::memory_order_relaxed);
                        return nullptr;
                    }
                    m_Bottom.store(bottom + 1, std::memory_order_relaxed);
                }
                return m_TaskBuf[bottom & (m_Cap - 1)].exchange(nullptr, std::memory_order_relaxed);
            }
            else
            {
                m_Bottom.store(bottom + 1, std::memory_order_relaxed);
                return nullptr;
            }
        }

        /// @brief
        /// Tries to retrieve a task from the front of the queue
        /// @param task Reference to a task that will be filled up, if the operation succeeds
        /// @return `true` if task was successfully retrieved, `false` otherwise
        T* try_steal()
        {
            size_t top = m_Top.load(std::memory_order_relaxed);
            size_t bottom = m_Bottom.load(); // Ceq_cst enforces total order with the thread stolen from

            // Empty?
            if (bottom - top <= 0)
                return nullptr;

            // Make sure it was not consumed in the mean time
            if (m_Top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
            {
                T* t = m_TaskBuf[top & (m_Cap - 1)].exchange(nullptr, std::memory_order_relaxed);
                return t;
            }

            return nullptr;
        }
    };
} // namespace tskr
