#pragma once

#include <concepts>
#include <atomic>

namespace tskr
{
    namespace impl
    {
        // Traits for function introspection, since the system needs to know the arguments and return type (if any) for proper scheduling
        template<typename>
        struct function_traits;

        // call operator
        template<typename T>
        struct function_traits : function_traits<decltype(&T::operator())> {};

        // const member function
        template<typename C, typename R, typename... Args>
        struct function_traits<R(C::*)(Args...) const>
        {
            using return_type = R;
            using args = std::tuple<Args...>;
        };

        // non-const member function
        template<typename C, typename R, typename... Args>
        struct function_traits<R(C::*)(Args...)>
        {
            using return_type = R;
            using args = std::tuple<Args...>;
        };

        // for free functions
        template<typename R, typename... Args>
        struct function_traits<R(*)(Args...)>
        {
            using return_type = R;
            using args = std::tuple<Args...>;
        };
    }

    struct Task
    {
        using Fn = void(*)(void*);

        Fn fun;
        void* data;
        std::atomic<int> deps{ 0 };
    };

    /// @brief 
    /// A wrapper around either a free function, callable object or a member function.
    /// Defines a single Task for execution
    template<auto Fn>
    struct TaskFn 
    {
        using task_traits = impl::function_traits<decltype(Fn)>;
        using args = typename task_traits::args;
    };

    /// @brief 
    /// Collection of Tasks and their dependencies
    /// @tparam TasksTuple Tasks to be ran in parallel
    /// @tparam AfterTuple Tasks in TasksTuple will be executed after the ones specified by AfterTuple
    /// @tparam BeforeTuple Tasks in TasksTuple will be executed before the ones specified by BeforeTuple
    template<typename TasksTuple, typename AfterTuple, typename BeforeTuple>
    struct TaskConfig
    {
        // TODO: Use with tasker to construct and run the actual tasks
        using tasks_t = TasksTuple;
        using after_t = AfterTuple;
        using before_t = BeforeTuple;

        template<typename... AfterTs>
        auto after(AfterTs...) const
        {
            // Concatenate the two lists to not lose the previous dependencies
            using merged_afters = decltype(std::tuple_cat(
                std::declval<AfterTuple>(),
                std::declval<std::tuple<AfterTs...>>()
            ));
            return TaskConfig<TasksTuple, merged_afters, BeforeTuple>{};
        }

        template<typename... BeforeTs>
        auto before(BeforeTs...) const
        {
            // Concatenate the two lists to not lose the previous dependencies
            using merged_befores = decltype(std::tuple_cat(
                std::declval<BeforeTuple>(),
                std::declval<std::tuple<BeforeTs...>>()
            ));
            return TaskConfig<TasksTuple, AfterTuple, merged_befores>{};
        }

    };

    /// @brief
    /// Wrapper for the comma operator and just a bunch of functions without dependencies
    template<typename... Ts>
    using TaskConfigBase = TaskConfig<
        std::tuple<Ts...>,
        std::tuple<>,
        std::tuple<>
    >;
} // namespace tskr

/// @brief
///Syntax sugar so that `(TaskFn<fun1>, TaskFn<fun2>)` returns a TaskConfig for chaining before and after calls
template<typename A, typename B >
constexpr auto operator,(A, B)
{
    return tskr::TaskConfigBase<A, B>{};
}