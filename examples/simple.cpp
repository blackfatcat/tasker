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

void task3()
{
    assert(done1.load(std::memory_order_acquire) && done2.load(std::memory_order_acquire));
    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
    done3.store(true, std::memory_order_release);
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

void task1()
{
    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }

    done1.store(true, std::memory_order_release);
}

void task4()
{
    assert(done1.load(std::memory_order_acquire) && done2.load(std::memory_order_acquire));
    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
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

void task7()
{
    assert(done3.load(std::memory_order_acquire) && done4.load(std::memory_order_acquire));
    assert(done5.load(std::memory_order_acquire) && done6.load(std::memory_order_acquire));

    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
}

int main() 
{
    tskr::Tasker tasker;

    tasker.add_schedules<Startup, Parallel<Main, Render>, Shutdown>(tskr::ExecutionPolicy::Repeat);

    tasker.add_tasks<Startup>((tskr::TaskFn<task1>{}, tskr::TaskFn<task2>{}));

    tasker.add_tasks<Main>(tskr::TaskFn<task4>{}.after(tskr::TaskFn<task3>{}));
    tasker.add_tasks<Render>(tskr::TaskFn<task6>{}.after(tskr::TaskFn<task5>{}));

    tasker.add_tasks<Shutdown>(tskr::TaskFn<task7>{});

    tasker.run();

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);

    tasker.halt();

    return 0;
}