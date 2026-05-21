#include "commands.hpp"

namespace tskr
{
    Commands::Commands(std::shared_ptr<WorkerPool>& pool, std::shared_ptr<ResourceStore>& resources) : m_WorkerPool(pool)
    {
        m_TaskContext.store = resources;
    }
    
    Commands::~Commands()
    {
    }
    void Commands::unalive_task(size_t task_id)
    {
        if (m_TasksAlive.contains(task_id))
            m_TasksAlive.erase(task_id);
    }
} // namespace tskr
