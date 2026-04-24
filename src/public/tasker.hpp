#pragma once

#include <vector>
#include <unordered_map>

#include "task.hpp"
#include "scheduler.hpp"
#include "types.hpp"
#include "worker_pool.hpp"
#include "resource.hpp"
#include "commands.hpp"

namespace tskr
{
    class Tasker
    {
    private:
        std::vector<std::vector<KEY_TYPE>> m_ScheduleHashes;
        std::unordered_map<KEY_TYPE, std::pair<ScheduleInfo, std::vector<std::shared_ptr<TaskNode>>>> m_TasksPerSchedule;
        std::shared_ptr<WorkerPool> m_Workers;

        // TODO: Move this to per-schedule instance + sync betweem non-parallel schedules
        std::shared_ptr<ResourceStore> m_Resources;

        std::atomic_bool m_Running{ true };
    public:
        Tasker(uint8_t thread_count = std::thread::hardware_concurrency(), size_t per_worker_cap = 256);
        ~Tasker();

        /// @brief
        /// @tparam ...Schedules a pack of unique types that will be registered as schedules. 
        /// @note The order the schedules are typed in will be the order they will be executed in as well,
        /// unless preceeded with Parallel<> in which case the schedules inside it will all be ran together
        /// @return reference to self for chaining
        template<typename ...Schedules>
        Tasker& add_schedules(ExecutionPolicy policy, size_t affinity_mask = 0xFFFFFFFFFFFFFFFF, uint16_t max_cores = 0)
        {
            m_ScheduleHashes.reserve(sizeof...(Schedules));
            m_TasksPerSchedule.reserve(sizeof...(Schedules));

            // Gets the hash of each schedule and inserts it into the vector through fold magic
            // Unfolds any nested declaration like Parallel<>
            (process_schedule_types<Schedules>(policy, affinity_mask, max_cores), ...);

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
            KEY_TYPE schedule_id = typeid(Schedule).accessor();
            assert(m_TasksPerSchedule.contains(schedule_id) && "Schedule deos not exist. Add it first with add_schedules.");

            ScheduleInfo& schedule_info = m_TasksPerSchedule.at(schedule_id).first;

            using tasks_ts = typename TaskConfig<Tasks...>::tasks_t;
            using after_ts = typename TaskConfig<Tasks...>::after_t;
            using before_ts = typename TaskConfig<Tasks...>::before_t;

            std::unordered_map<KEY_TYPE, std::shared_ptr<TaskNode>> map = TaskNode::build_node_map(tasks, m_Resources, schedule_info);

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

        template<typename T>
        Tasker& register_resource(T&& resource)
        {
            m_Resources->insert(std::move(resource));
            return *this;
        }

        /// @brief 
        /// Kick off the execution of the schedules and tasks
        void run();

        /// @brief 
        /// Shut down all work
        void halt();

        /// @brief Prints the graph in a readdable format either to the console (if no path is provided) or to the file provided in the .dot format
        /// @param filename relative to the current working directory path to the file to which to dump a .dot representation of the task graph
        void print_graph(std::string_view filename = "");

    private:
        template <typename T>
        void process_schedule_types(ExecutionPolicy policy, size_t affinity_mask, uint16_t max_cores)
        {
            std::vector<KEY_TYPE> par_schedules{};
            std::shared_ptr<std::atomic_bool> repeating = std::make_shared<std::atomic_bool>(false);

            // TODO: Run these in parallel
            if constexpr (impl::is_parallel<T>::value)
            {
                // T is Parallel<A,B,C>
                std::apply([this, &par_schedules, policy, &repeating, affinity_mask, max_cores](auto... inner) {
                    (par_schedules.push_back(typeid(inner).accessor()), ...);
                    
                    if (policy == ExecutionPolicy::Repeat)
                        repeating->store(true, std::memory_order_relaxed);
                    
                    (m_Resources->insert(Repeating<decltype(inner)>(repeating)), ...);
                    (m_TasksPerSchedule.emplace(typeid(inner).accessor(), std::make_pair(ScheduleInfo(policy, repeating, typeid(inner).name(), affinity_mask, max_cores), std::vector<std::shared_ptr<TaskNode>>{})), ...);
                }, typename impl::parallel_inner<T>::types{});
            }
            else
            {
                // T is a single schedule
                par_schedules.push_back(typeid(T).accessor());
                
                if (policy == ExecutionPolicy::Repeat)
                    repeating->store(true, std::memory_order_relaxed);
                
                m_Resources->insert(Repeating<T>(repeating));
                m_TasksPerSchedule.emplace(typeid(T).accessor(), std::make_pair(ScheduleInfo(policy, repeating, typeid(T).name(), affinity_mask, max_cores), std::vector<std::shared_ptr<TaskNode>>{}));
            }
            m_ScheduleHashes.push_back(par_schedules);
        }
    };
} // namespace tskr
