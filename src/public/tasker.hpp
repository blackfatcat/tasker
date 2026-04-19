#include <vector>

#include "task.hpp"
#include "scheduler.hpp"
#include "types.hpp"
#include "worker_pool.hpp"

namespace tskr
{
    class Tasker
    {
    private:
        std::vector<std::size_t> m_ScheduleHashes;
        std::vector<std::size_t> m_ParallelScheduleHashes;

        WorkerPool workers;
    public:
        Tasker(/* args */);
        ~Tasker();
        // TODO: introduce an execution policy (single, repeated - to simulate a game loop, etc.) 
        /// @brief 
        /// @tparam ...Schedules a pack of unique types that will be registered as schedules. 
        /// @note The order the schedules are typed in will be the order they will be executed in as well,
        /// unless preceeded with Parallel<> in which case the schedules inside it will all be ran together
        /// @return reference to self for chaining
        template<typename ...Schedules>
        Tasker& add_schedules() 
        {
            m_ScheduleHashes.reserve(sizeof...(Schedules));
            m_ParallelScheduleHashes.reserve(sizeof...(Schedules));

            // Gets the hash of each schedule and inserts it into the vector through fold magic
            // Unfolds any nested declaration like Parallel<>
            (process_schedule_types<Schedules>(), ...);

            return *this;
        }

        /// @brief 
        /// @tparam Schedule unique type, registered as a schedule with `add_schedule`
        /// @param tasks a task config object that can be constructed with a bunch of functions or collable objects
        /// @note Tasks can be organised within a `TaskConfig` with calls to before() and after(), if not, the scheduler will try to execute them in parallel.
        /// See (TODO: insert git link) for examples
        /// @return reference to self for chaining
        template<typename Schedule, typename ...Tasks>
        Tasker& add_tasks(TaskConfig<Tasks...> tasks) { return *this; }

        /// @brief 
        /// Kick off the execution of the schedules and tasks
        void run();

        /// @brief 
        /// Shut down all work
        void halt();

    private:
        template <typename T>
        void process_schedule_types()
        {
            // TODO: Run these in parallel
            if constexpr (impl::is_parallel<T>::value)
            {
                // T is Parallel<A,B,C>
                std::apply([this](auto... inner){
                    (m_ScheduleHashes.push_back(typeid(inner).hash_code()), ...);
                    (m_ParallelScheduleHashes.push_back(typeid(inner).hash_code()), ...);
                }, typename impl::parallel_inner<T>::types{});
            }
            else
            {
                // T is a single schedule
                m_ScheduleHashes.push_back(typeid(T).hash_code());
            }
        }
    };    
} // namespace tskr
