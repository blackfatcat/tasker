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
        m_Workers.work();

        bool first_run = true;
        bool repeating = true;

        // Main loop
        while (m_Running.load(std::memory_order_relaxed) && repeating)
        {
            repeating = false;
            for (auto& schedule_set : m_ScheduleHashes)
            {
                for (auto& schedule : schedule_set)
                {
                    std::pair<ExecutionPolicy, std::vector<std::shared_ptr<TaskNode>>>& tasks = m_TasksPerSchedule[schedule];
                    if (tasks.first == ExecutionPolicy::Repeat || first_run)
                    {
                        repeating = true;

                        // Tasks are running already, so this atomic add will prevent the scenario where a task is executed,
                        // decreasing the counter to 0 and falsly signalling that there are no more left when the rest have just not been enqueued
                        m_Workers.add_task_count(tasks.second.size());
                        for (auto& task : tasks.second)
                        {
                            if (task->deps_remaining.load(std::memory_order_acquire) <= 0)
                                m_Workers.enqueue(task);
                        }
                    }
                }
                m_Workers.wait_for_all();
            }
            first_run = false;
        }
    }

    void Tasker::halt()
    {
        m_Running.store(false);
        m_Workers.wait_for_all();
        m_Workers.stop();
    }
}