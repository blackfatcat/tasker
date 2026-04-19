#include <iostream>

#include "tasker.hpp"

struct Startup {};
struct Main {};
struct Render {};
struct Shutdown {};

using tskr::Parallel;

void task3(const std::vector<int>& in)
{
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
    std::cout << "Total: " << total << std::endl;
}


void task2(const std::vector<int>& in)
{
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
    std::cout << "Total: " << total << std::endl;
}

void task1(const std::vector<int>& in)
{
    int total = 0;
    for (const auto& i : in)
    {
        total += i;
    }
    std::cout << "Total: " << total << std::endl;
}

int main() 
{
    tskr::Tasker tasker;

    tasker.add_schedules<Startup, Parallel<Main, Render>, Shutdown>();
    tasker.add_tasks<Startup>((tskr::TaskFn<task1>{}, tskr::TaskFn<task2>{}).after(tskr::TaskFn<task3>{}));

    std::cout << "Scheduled" << std::endl;

    return 0;
}