#pragma once

#include <memory>

#include "worker_pool.hpp"
#include "types.hpp"
#include "task.hpp"

namespace tskr
{
    namespace impl
    {
        static std::atomic_int s_NextId = std::atomic_int{ 0 };
    } // namespace impl

    /// @brief Allows for various operations from within a task.
    /// @brief Can be queried as a param to a task: `task(Commands cmd){}`
    class Commands
    {
    private:
        std::shared_ptr<WorkerPool> m_WorkerPool;
        std::unordered_map<size_t, std::shared_ptr<TaskNode>> m_TasksAlive;

        TaskContext m_TaskContext;

    public:
        Commands() {}
        Commands(std::shared_ptr<WorkerPool>& pool, std::shared_ptr<ResourceStore>& resources);
        ~Commands();

        template<typename Fn>
        void spawn(Fn f)
        {
            ScheduleInfo info = m_TaskContext.store->get_ref<ScheduleInfo>();
            m_TaskContext.schedule_info = info;

            std::shared_ptr<TaskNode> node = TaskNode::make_from_taskfn(f, m_TaskContext);
            node->id = impl::s_NextId.load(std::memory_order_acquire);

            // Keep task alive until it is executed
            m_TasksAlive.emplace(node->id, node);

            switch (Fn::task_type)
            {
            case TaskSpawnType::Standalone:
                m_WorkerPool->enqueue(node.get(), false);
                break;
            case TaskSpawnType::Scheduled:
                m_WorkerPool->enqueue(node.get(), true);
                break;
            default:
                break;
            }

            impl::s_NextId.fetch_add(1, std::memory_order_release);
        }

        void unalive_task(size_t task_id);
    };

    /// @brief Specialization for the Commands class, which allows for a direct access within a task
    template<>
    struct ParamFetcher<Commands>
    {
        static Commands& fetch(TaskContext& ctx, int)
        {
            return ctx.store->get_ref<Commands>();
        }
    };
} // namespace tskr
