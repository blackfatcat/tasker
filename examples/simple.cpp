#include <iostream>

#include "scheduler.hpp"

int main() 
{
    tskr::scheduler sched {1};
    std::cout << "Scheduled" << std::endl;
    return 0;
}