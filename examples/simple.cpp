#include <iostream>
#include <chrono>

#include "tasker.hpp"

struct Startup {};
struct Main {};
struct Render {};
struct Sync {};
struct Shutdown {};

using tskr::Parallel;

static std::atomic_bool done1{ false };
static std::atomic_bool done2{ false };
static std::atomic_bool done3{ false };
static std::atomic_bool done4{ false };
static std::atomic_bool done5{ false };
static std::atomic_bool done6{ false };
static std::atomic_bool done7{ false };
static std::atomic_bool done8{ false };
static std::atomic_bool done9{ false };
static std::atomic_bool done10{ false };

struct VecRes
{
    std::vector<int> vec{ 1,2,3,4,5,6 };
};

void sync()
{
    assert(done5.load(std::memory_order_acquire) && done6.load(std::memory_order_acquire) && done7.load(std::memory_order_acquire));
    std::cout << "Sync" << std::endl;
    done1.store(true, std::memory_order_release);
}

void task_inner(tskr::Resource<VecRes> res)
{
    static_cast<void*>(&res);

    res->vec[0] = 7;
}

void task1(tskr::Commands commands)
{
    commands.spawn(tskr::TaskFn<task_inner, tskr::TaskSpawnType::Standalone>{});

    std::cout << "Start" << std::endl;
    done1.store(true, std::memory_order_release);
}

void task2()
{
    done2.store(true, std::memory_order_release);
}

void task3(tskr::Resource<VecRes> vec_res)
{
    assert(done1.load(std::memory_order_acquire) && done2.load(std::memory_order_acquire));

    vec_res->vec.push_back(7);
    done3.store(true, std::memory_order_release);
}

void task4(tskr::Resource<VecRes> vec_res)
{
    assert(done1.load(std::memory_order_acquire) && done2.load(std::memory_order_acquire));
    for (const auto& i : vec_res->vec)
    {
        std::cout << i << std::endl;
    }
    done4.store(true, std::memory_order_release);
}

void task5()
{
    assert(done1.load(std::memory_order_acquire) && done2.load(std::memory_order_acquire));
    done5.store(true, std::memory_order_release);
}

void task6()
{
    assert(done1.load(std::memory_order_acquire) && done2.load(std::memory_order_acquire));
    done6.store(true, std::memory_order_release);
}

void task7(
    tskr::Resource<tskr::Repeating<Main>> main_schedule_repeating,
    tskr::Resource<tskr::Repeating<Sync>> sync_schedule_repeating
)
{
    assert(done1.load(std::memory_order_acquire) && done2.load(std::memory_order_acquire));

    // Stops Main and Render from repeating, terminating the program
    //main_schedule_repeating->stop();
    //sync_schedule_repeating->stop();

    done7.store(true, std::memory_order_release);
}

void task8()
{
    assert(done5.load(std::memory_order_acquire) && done6.load(std::memory_order_acquire) && done7.load(std::memory_order_acquire));
    done8.store(true, std::memory_order_release);
}

void task9()
{
    assert(done5.load(std::memory_order_acquire) && done6.load(std::memory_order_acquire) && done7.load(std::memory_order_acquire));
    done9.store(true, std::memory_order_release);
}

void task10()
{
    assert(done5.load(std::memory_order_acquire) && done6.load(std::memory_order_acquire) && done7.load(std::memory_order_acquire));
    done10.store(true, std::memory_order_release);
}

void task11()
{
    assert(done8.load(std::memory_order_acquire) && done9.load(std::memory_order_acquire) && done10.load(std::memory_order_acquire));
    std::cout << "Done" << std::endl;
}

int main()
{
    tskr::Tasker tasker;

    // Startup schedule executes first, just once
    tasker.add_schedules<Startup>(tskr::ExecutionPolicy::Single)

        // Then follow Main and Render in Parallel and Sync after the two finish, the 3 of them repeating, until stopped
        .add_schedules<Parallel<Main, Render>, Sync>(tskr::ExecutionPolicy::Repeat, 4, 0b0011)

        // Last comes Shutdown, once
        .add_schedules<Shutdown>(tskr::ExecutionPolicy::Single)

        // task1 and 2 run in parallel during Startup
        .add_tasks<Startup>((tskr::TaskFn<task1>{}, tskr::TaskFn<task2>{}))

        // task4 runs after task3 in Main
        .add_tasks<Main>(tskr::TaskFn<task4>{}.after(tskr::TaskFn<task3>{}))

        // task6 and 7 run in parallel after 5 finishes in Render
        .add_tasks<Render>((tskr::TaskFn<task6>{}, tskr::TaskFn<task7>{}).after(tskr::TaskFn<task5>{}))

        // task sync runs after both Main and Render have finished
        .add_tasks<Sync>(tskr::TaskFn<sync>{})

        // task8, 9 and 10 run in parallel followed by 11 after all have finished
        .add_tasks<Shutdown>((tskr::TaskFn<task8>{}, tskr::TaskFn<task9>{}, tskr::TaskFn<task10>{}).before(tskr::TaskFn<task11>{}))

        // Register a custom resource
        .register_resource(VecRes{})

        // Run the graph
        .run();

    return 0;
}