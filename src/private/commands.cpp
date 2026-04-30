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
} // namespace tskr
