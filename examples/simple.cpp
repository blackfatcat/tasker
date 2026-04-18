#include <iostream>

#include "tasker.hpp"

struct Startup {};
struct Main {};
struct Render {};
struct Shutdown {};

using tskr::Parallel;

struct Task1
{
    void operator()(const std::vector<int>& in) const
    {
        int total = 0;
        for (const auto& i : in)
        {
            total += i;
        }
        std::cout << "Total: " << total << std::endl;
    }
};

struct Task2
{
    void operator()(const std::vector<int>& in) const
    {
        int total = 0;
        for (const auto& i : in)
        {
            total += i;
        }
        std::cout << "Total: " << total << std::endl;
    }
};

struct Task3
{
    void operator()(const std::vector<int>& in) const
    {
        int total = 0;
        for (const auto& i : in)
        {
            total += i;
        }
        std::cout << "Total: " << total << std::endl;
    }
};

int main() 
{
    tskr::Tasker tasker;

    tasker.add_schedules<Startup, Parallel<Main, Render>, Shutdown>();
    tasker.add_tasks<Startup>((tskr::TaskFn<Task1{}>{}, tskr::TaskFn<Task2{}>{}).after(tskr::TaskFn<Task3{}>{}));

    std::cout << "Scheduled" << std::endl;

    return 0;
}