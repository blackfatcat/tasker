#include "commands.hpp"

namespace tskr
{
    Commands::Commands(std::shared_ptr<WorkerPool> pool, std::shared_ptr<ResourceStore> resources) : m_WorkerPool(pool), m_Resources(resources)
    {
    }
    
    Commands::~Commands()
    {
    }
} // namespace tskr
