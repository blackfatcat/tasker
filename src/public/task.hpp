#pragma once

#include <concepts>
#include <atomic>
#include <unordered_map>
#include <tuple>
#include <memory>

namespace tskr
{
    using KEY_TYPE = const char*;

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

        template<typename Tuple, typename F, std::size_t... I>
        void for_each_in_tuple_impl(Tuple&& t, F&& f, std::index_sequence<I...>)
        {
            (f(std::get<I>(t)), ...);
        }

        template<typename Tuple, typename F>
        void for_each_in_tuple(Tuple&& t, F&& f)
        {
            constexpr auto N = std::tuple_size_v<std::remove_reference_t<Tuple>>;
            for_each_in_tuple_impl(std::forward<Tuple>(t), std::forward<F>(f),
                std::make_index_sequence<N>{});
        }
    }

    /// @brief The backbone of the system - a single point of execution tied to a function
    struct Task
    {
        using Fn = void(*)(void*);

        Fn fun;
        void* payload;
        std::atomic<int> deps{ 0 };
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

        /// @brief Schedule these functions `after` all specified with this call have finished
        /// @param AfterTs Functions to be executed before the ones in the TaskConfig
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

        /// @brief Schedule these functions `before` all specified with this call have finished
        /// @param AfterTs Functions to be executed after the ones in the TaskConfig
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

    /// @brief
    /// The bridge between type-space and run-time Tasks
    template<typename Fn>
    struct TaskInvoker;

    // no args
    // TODO: Store a tuple in data and unpack?
    template<typename R>
    struct TaskInvoker<R(*)()>
    {
        using Fun = R(*)();

        static void invoke(void* payload)
        {
            auto fun = reinterpret_cast<Fun>(payload);
            (*fun)();
        }
    };

    /// @brief 
    /// A compile-time info carrier for either a free function, callable object or a member function.
    template<auto Fn>
    struct TaskFn
    {
        using task_traits = impl::function_traits<decltype(Fn)>;
        using fun_type = decltype(Fn);
        using args = typename task_traits::args;
        using return_type = typename task_traits::return_type;

        static std::unique_ptr<Task> make_task()
        {
            auto t = std::make_unique<Task>();
            t->fun = &TaskInvoker<fun_type>::invoke;
            t->payload = reinterpret_cast<void*>(Fn);
            return t;
        }

        /// @brief Schedule this function `after` all specified with this call have finished
        /// @param AfterTs Functions to be executed before this one
        template<typename... AfterTs>
        auto after(AfterTs...) const
        {
            return TaskConfig<
                std::tuple<TaskFn<Fn>>,
                std::tuple<AfterTs...>,
                std::tuple<>
            >{};
        }

        /// @brief Schedule this function `before` all specified with this call have finished
        /// @param AfterTs Functions to be executed after this one
        template<typename... BeforeTs>
        auto before(BeforeTs...) const
        {
            return TaskConfig<
                std::tuple<TaskFn<Fn>>,
                std::tuple<>,
                std::tuple<BeforeTs...>>{};
        }
    };

    /// @brief Task + dependencies
    struct TaskNode
    {
        std::unique_ptr<Task> task = nullptr;
        std::vector<TaskNode*> dependents; // outgoing edges
        std::atomic<int> deps_remaining;   // incoming edges

        template<typename TaskFnT>
        static TaskNode* make_from_taskfn(TaskFnT task)
        {
            TaskNode* node = new TaskNode;
            node->task = task.make_task();
            node->deps_remaining.store(0, std::memory_order_relaxed);
            return node;
        }

        template<typename... Ts>
        static std::unordered_map<KEY_TYPE, TaskNode*> build_node_map(std::tuple<Ts...> tasks)
        {
            std::unordered_map<KEY_TYPE, TaskNode*> map;

            impl::for_each_in_tuple(tasks, [&](auto task) {
                map.emplace(typeid(task).name(), TaskNode::make_from_taskfn(task));
            });

            return map;
        }

        template<typename... Ts>
        void wire_dependencies(TaskConfig<Ts...> cfg, std::unordered_map<KEY_TYPE, TaskNode*>& map)
        {
            using tasks_ts = typename TaskConfig<Ts...>::tasks_t;
            using after_ts = typename TaskConfig<Ts...>::after_t;
            using before_ts = typename TaskConfig<Ts...>::before_t;

            // Something like (TaskA, TaskB).after(TaskX, TaskY) needs to build edges like:
            // TaskX -> TaskA
            // TaskX -> TaskB
            // TaskY -> TaskA
            // TaskY -> TaskB
            // So, For each after in after_t and for each task in tasks_t
            // insert dependent for the 'after' task
            // increase dependency count for task
            impl::for_each_in_tuple(after_ts{}, [&](auto after_t) {
                impl::for_each_in_tuple(tasks_ts{}, [&](auto task_t) {
                    TaskNode* after = map[typeid(after_t).name()];
                    TaskNode* task = map[typeid(task_t).name()];

                    after->dependents.push_back(task);
                    task->deps_remaining.fetch_add(1, std::memory_order_relaxed);
                });
            });

            // Do the opposite for before_ts
            impl::for_each_in_tuple(before_ts{}, [&](auto before_t) {
                impl::for_each_in_tuple(tasks_ts{}, [&](auto task_t) {
                    TaskNode* before = map[typeid(before_t).name()];
                    TaskNode* task = map[typeid(task_t).name()];

                    task->dependents.push_back(before);
                    before->deps_remaining.fetch_add(1, std::memory_order_relaxed);
                });
            });
            
        }
    };

} // namespace tskr

/// @brief
/// Syntax sugar so that `(TaskFn<fun1>, TaskFn<fun2>)` returns a TaskConfig for chaining before and after calls
template<typename A, typename B >
constexpr auto operator,(A, B)
{
    return tskr::TaskConfigBase<A, B>{};
}