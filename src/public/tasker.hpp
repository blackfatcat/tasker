#pragma once

#include <vector>
#include <unordered_map>

#include "task.hpp"
#include "scheduler.hpp"
#include "types.hpp"
#include "worker_pool.hpp"

namespace tskr
{
    enum class ExecutionPolicy : uint8_t
    {
        Single,
        Repeat
    };

    class Tasker
    {
    private:
        std::vector<std::vector<size_t>> m_ScheduleHashes;
        std::unordered_map<size_t, std::pair<ExecutionPolicy, std::vector<std::shared_ptr<TaskNode>>>> m_TasksPerSchedule;
        WorkerPool m_Workers;

        std::atomic_bool m_Running{ true };
    public:
        Tasker();
        ~Tasker();

        /// @brief
        /// @tparam ...Schedules a pack of unique types that will be registered as schedules. 
        /// @note The order the schedules are typed in will be the order they will be executed in as well,
        /// unless preceeded with Parallel<> in which case the schedules inside it will all be ran together
        /// @return reference to self for chaining
        template<typename ...Schedules>
        Tasker& add_schedules(ExecutionPolicy policy) 
        {
            m_ScheduleHashes.reserve(sizeof...(Schedules));
            m_TasksPerSchedule.reserve(sizeof...(Schedules));

            // Gets the hash of each schedule and inserts it into the vector through fold magic
            // Unfolds any nested declaration like Parallel<>
            (process_schedule_types<Schedules>(policy), ...);

            return *this;
        }

        /// @brief 
        /// @tparam Schedule unique type, registered as a schedule with `add_schedule`
        /// @param tasks a task config object that can be constructed with a bunch of functions or collable objects
        /// @note Tasks can be organised within a `TaskConfig` with calls to before() and after(), if not, the scheduler will try to execute them in parallel.
        /// See (TODO: insert git link) for examples
        /// @return reference to self for chaining
        template<typename Schedule, typename ...Tasks>
        Tasker& add_tasks(TaskConfig<Tasks...> tasks)
        {
            size_t schedule_id = typeid(Schedule).hash_code();
            assert(m_TasksPerSchedule.contains(schedule_id) && "Schedule deos not exist. Add it first with add_schedules.");

            using tasks_ts = typename TaskConfig<Tasks...>::tasks_t;
            using after_ts = typename TaskConfig<Tasks...>::after_t;
            using before_ts = typename TaskConfig<Tasks...>::before_t;

            std::unordered_map<KEY_TYPE, std::shared_ptr<TaskNode>> map = TaskNode::build_node_map(tasks_ts{});
            TaskNode::wire_dependencies(tasks, map);

            std::vector<std::shared_ptr<TaskNode>>& task_nodes = m_TasksPerSchedule.at(schedule_id).second;

            for (auto& [key, node] : map)
            {
                task_nodes.push_back(node);
            }

            return *this;
        }

        template<typename Schedule, typename ...Tasks>
        Tasker& add_tasks(Tasks... tasks)
        {
            add_tasks<Schedule>(TaskConfigBase<Tasks...>{});
            return *this;
        }

        /// @brief 
        /// Kick off the execution of the schedules and tasks
        void run();

        /// @brief 
        /// Shut down all work
        void halt();

    private:
        template <typename T>
        void process_schedule_types(ExecutionPolicy policy)
        {
            std::vector<size_t> par_schedules{};
            // TODO: Run these in parallel
            if constexpr (impl::is_parallel<T>::value)
            {
                // T is Parallel<A,B,C>
                std::apply([this, &par_schedules, policy](auto... inner){
                    (par_schedules.push_back(typeid(inner).hash_code()), ...);
                    (m_TasksPerSchedule.emplace(typeid(inner).hash_code(), std::make_pair(policy, std::vector<std::shared_ptr<TaskNode>>{})), ...);
                }, typename impl::parallel_inner<T>::types{});
            }
            else
            {
                // T is a single schedule
                par_schedules.push_back(typeid(T).hash_code());
                m_TasksPerSchedule.emplace(typeid(T).hash_code(), std::make_pair(policy, std::vector<std::shared_ptr<TaskNode>>{}));
            }
            m_ScheduleHashes.push_back(par_schedules);
        }
    };    
} // namespace tskr
