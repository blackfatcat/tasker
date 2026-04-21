#include "tasker.hpp"

namespace tskr
{
    Tasker::Tasker(/* args */)
    {
    }

    Tasker::~Tasker()
    {
        halt();
    }

	void Tasker::run()
    {
        // TODO: Pass in resources
        workers.work();

        // Main loop
        while (true)
        {
            for (auto& schedule_set : m_ScheduleHashes)
            {
                for (auto& schedule : schedule_set)
                {
                    std::pair<ExecutionPolicy, std::vector<TaskNode*>>& tasks = m_TasksPerSchedule[schedule];
                    for (auto& task : tasks.second)
                    {
                        if (task->deps_remaining.load(std::memory_order_acquire) <= 0)
                            workers.enqueue(task);
                    }
                }
                workers.wait_for_all();
            }

        }
    }

    void Tasker::halt()
    {
        workers.wait_for_all();
        workers.stop();
    }
}