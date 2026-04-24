#pragma once

#include <memory>

#include "worker_pool.hpp"
#include "types.hpp"
#include "task.hpp"

namespace tskr
{
    /// @brief Allows for various operations from within a task.
    /// @brief Can be queried as a param to a task: `task(Commands cmd){}`
    class Commands
    {
    private:
        std::shared_ptr<WorkerPool> m_WorkerPool;
        std::shared_ptr<ResourceStore> m_Resources;
    public:
        Commands() {}
        Commands(std::shared_ptr<WorkerPool> pool, std::shared_ptr<ResourceStore> resources);
        ~Commands();

        template<typename Fn>
        void spawn(Fn f)
        {
            ScheduleInfo info = m_Resources->get_ref<ScheduleInfo>();
            switch (Fn::task_type)
            {
            case TaskSpawnType::Standalone:
                m_WorkerPool->enqueue(TaskNode::make_from_taskfn(f, m_Resources, info), false);
                break;
            case TaskSpawnType::Scheduled:
                m_WorkerPool->enqueue(TaskNode::make_from_taskfn(f, m_Resources, info), true);
                break;
            default:
                break;
            }
        }
    };

    /// @brief Specialization for the Commands class, which allows for a direct access within a task
    template<>
    struct ParamFetcher<Commands>
    {
        static Commands& fetch(std::shared_ptr<ResourceStore> store)
        {
            return store->get_ref<Commands>();
        }
    };
} // namespace tskr
