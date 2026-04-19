#include <cassert>

#include "queue.hpp"

namespace tskr
{
    WorkStealingDeque::WorkStealingDeque(size_t cap) 
        : m_Cap(cap),
        m_Top(0),
        m_Bottom(0)
    {
        m_TaskBuf.reserve(m_Cap);
    }
    
    WorkStealingDeque::~WorkStealingDeque()
    {
        m_TaskBuf.clear();
    }

    void WorkStealingDeque::push(Task* task)
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

    bool WorkStealingDeque::try_push(Task* task)
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

    bool WorkStealingDeque::try_pop(Task* task)
    {
        size_t bottom = m_Bottom.load(std::memory_order_relaxed);
        size_t top = m_Top.load(std::memory_order_relaxed);

        // Nothing left
        if (bottom == top)
            return false;

        --bottom;
        m_Bottom.store(bottom); // Ceq_cst enforeces total order with thieves's load

        Task* t = m_TaskBuf[bottom & (m_Cap - 1)]->load(std::memory_order_relaxed);

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

    bool WorkStealingDeque::try_steal(Task* task)
    {
        size_t top = m_Top.load(std::memory_order_relaxed);
        size_t bottom = m_Bottom.load(); // Ceq_cst enforces total order with the thread stolen from
        
        // Empty?
        if (bottom - top <= 0)
            return false;

        Task* t = m_TaskBuf[top & (m_Cap - 1)]->load(std::memory_order_relaxed);

        // Make sure it was not consumed in the mean time
        if (m_Top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
        {
            task = t;
            return true;
        }

        return false;
    }

} // namespace tskr
