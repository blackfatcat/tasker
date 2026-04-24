#include <iostream>
#include <chrono>

#include "tasker.hpp"

struct Startup {};
struct Main {};
struct Render {};
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

void task_inner()
{
    std::cout << "Inner "<< std::endl;
}

void task1(tskr::Commands commands, tskr::Resource<tskr::ScheduleInfo> info)
{
    commands.spawn(tskr::TaskFn<task_inner, tskr::TaskSpawnType::Standalone>{});

    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }

    done1.store(true, std::memory_order_release);
}

void task2()
{
    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }

    done2.store(true, std::memory_order_release);
}

void task3(tskr::Resource<VecRes> vec_res)
{
    assert(done1.load(std::memory_order_acquire) && done2.load(std::memory_order_acquire));

    vec_res->vec.push_back(7);
    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
    
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
    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
    done5.store(true, std::memory_order_release);
}

void task6()
{
    assert(done1.load(std::memory_order_acquire) && done2.load(std::memory_order_acquire));
    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
    done6.store(true, std::memory_order_release);
}

void task7(
    tskr::Resource<tskr::Repeating<Main>> main_schedule_repeating
)
{
    assert(done1.load(std::memory_order_acquire) && done2.load(std::memory_order_acquire));

    // Stops Main and Render from repeating, terminating the program
    main_schedule_repeating->stop();

    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
    done7.store(true, std::memory_order_release);
}

void task8()
{
    assert(done5.load(std::memory_order_acquire) && done6.load(std::memory_order_acquire) && done7.load(std::memory_order_acquire));

    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
    done8.store(true, std::memory_order_release);
}

void task9()
{
    assert(done5.load(std::memory_order_acquire) && done6.load(std::memory_order_acquire) && done7.load(std::memory_order_acquire));

    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
    done9.store(true, std::memory_order_release);
}

void task10()
{
    assert(done5.load(std::memory_order_acquire) && done6.load(std::memory_order_acquire) && done7.load(std::memory_order_acquire));

    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
    done10.store(true, std::memory_order_release);
}

void task11()
{
    assert(done8.load(std::memory_order_acquire) && done9.load(std::memory_order_acquire) && done10.load(std::memory_order_acquire));

    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }

    std::cout << "Done" << std::endl;
}

int main()
{
    tskr::Tasker tasker;

    tasker.add_schedules<Startup>(tskr::ExecutionPolicy::Single)
        .add_schedules<Parallel<Main, Render>>(tskr::ExecutionPolicy::Repeat, 4, 0b0011)
        .add_schedules<Shutdown>(tskr::ExecutionPolicy::Single)
        .add_tasks<Startup>((tskr::TaskFn<task1>{}, tskr::TaskFn<task2>{}))
        .add_tasks<Main>(tskr::TaskFn<task4>{}.after(tskr::TaskFn<task3>{}))
        .add_tasks<Render>((tskr::TaskFn<task6>{}, tskr::TaskFn<task7>{}).after(tskr::TaskFn<task5>{}))
        .add_tasks<Shutdown>((tskr::TaskFn<task8>{}, tskr::TaskFn<task9>{}, tskr::TaskFn<task10>{}).before(tskr::TaskFn<task11>{}))
        .register_resource(VecRes{})
        .run();

    return 0;
}