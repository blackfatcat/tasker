#pragma once

#include <type_traits>
#include <memory>
#include <tuple>

namespace tskr
{
    /// @brief Define schedules to be executed in parallel
    /// @tparam Ts a pack of types that will mark schedules for parallel execution
    template<typename... Ts>
    struct Parallel {};

    namespace impl {
        template <typename T>
        struct is_parallel : std::false_type {};

        template <typename ...Ts>
        struct is_parallel<Parallel<Ts...>> : std::true_type {};

        template <typename T>
        struct parallel_inner;

        template <typename... Ts>
        struct parallel_inner<Parallel<Ts...>>
        {
            using types = std::tuple<Ts...>;
        };
    }

    /// Determines how the task should be ran
    enum class TaskSpawnType : uint8_t
    {
        Scheduled,  // Task will be added to the current schedule and waited on
        Standalone, // Standalone task, will not be waited on
    };

    /// @brief Define how a schedule should be executed
    enum class ExecutionPolicy : uint8_t
    {
        Single, // Run once
        Repeat  // Run repeatedly, until signalled to stop
    };

    /// @brief Determines if the whole app is running
    struct Running
    {
        Running()
        {
            m_Running = std::make_shared<std::atomic_bool>(true);
        }
        void stop()
        {
            m_Running->store(false, std::memory_order_relaxed);
        }
        bool is_running()
        {
            return m_Running->load(std::memory_order_relaxed);
        }
    private:
        std::shared_ptr<std::atomic_bool> m_Running;
    };    

    /// @brief Determines if a schedule should continue repeating.
    /// @brief Registered as Resource<Repeating<ScheduleType>> for each registered schedule.
    /// @brief Initialized with true if the Schedule is repeating
    /// @brief Can be used within a task to determine when the specified schedule should start/stop repeating
    template<typename Schedule>
    struct Repeating
    {
        Repeating(std::shared_ptr<std::atomic_bool> repeating) : m_Repeating(repeating)
        {
        }
        void start()
        {
            m_Repeating->store(true, std::memory_order_relaxed);
        }
        void stop()
        {
            m_Repeating->store(false, std::memory_order_relaxed);
        }
        bool is_repeating()
        {
            return m_Repeating->load(std::memory_order_relaxed);
        }
    private:
        std::shared_ptr<std::atomic_bool> m_Repeating;
    };

    struct ScheduleInfo
    {
        ScheduleInfo() : policy(ExecutionPolicy::Single), repeating(std::make_shared<std::atomic_bool>(false)) {}
        ScheduleInfo(ExecutionPolicy pol, std::shared_ptr<std::atomic_bool> rep) : policy(pol), repeating(rep) {}

        ExecutionPolicy policy;

        // Note: Shared between Parallel schedules
        std::shared_ptr<std::atomic_bool> repeating;
    };
} // namespace tskr
