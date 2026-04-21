#include <iostream>
#include <chrono>

#include "tasker.hpp"

struct Startup {};
struct Main {};
struct Render {};
struct Shutdown {};

using tskr::Parallel;

void task3()
{
    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
}


void task2()
{
    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
}

void task1()
{
    const std::vector<int>& in{ 1,2,3,4,5,6 };
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
}

int main() 
{
    //std::cout << "Task 1: " << typeid(&task1).name() << std::endl << "Task 2: " << typeid(&task2).name() << std::endl << "Task 3: " << typeid(&task3).name() << std::endl;

    tskr::Tasker tasker;

    tasker.add_schedules<Startup>(tskr::ExecutionPolicy::Single);
    tasker.add_schedules<Parallel<Main, Render>>(tskr::ExecutionPolicy::Repeat);
    tasker.add_schedules<Shutdown>(tskr::ExecutionPolicy::Single);

    tasker.add_tasks<Startup>((tskr::TaskFn<task1>{}, tskr::TaskFn<task2>{}));

    tasker.add_tasks<Main>(tskr::TaskFn<task2>{}.after(tskr::TaskFn<task1>{}));
    tasker.add_tasks<Render>(tskr::TaskFn<task2>{}.after(tskr::TaskFn<task1>{}));

    tasker.add_tasks<Shutdown>(tskr::TaskFn<task3>{});

    tasker.run();

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);

    tasker.halt();

    return 0;
}