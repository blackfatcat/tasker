#include "tasker.hpp"

namespace tskr
{
    Tasker::Tasker(/* args */)
    {
        m_Resources.insert(Running{});
    }

    Tasker::~Tasker()
    {
        halt();
    }

	void Tasker::run()
    {
        // TODO: Pass in resources
        m_Workers.work();

        bool repeating = true;

        // Main loop
        while (m_Resources.get<Running>()->is_running() && repeating)
        {
            repeating = false;
            //for (auto& schedule_set : m_ScheduleHashes)
            for (size_t schedule_set_idx = 0; schedule_set_idx < m_ScheduleHashes.size();)
            {
                std::vector<std::size_t>& schedule_set = m_ScheduleHashes[schedule_set_idx];

                for (auto& schedule : schedule_set)
                {
                    std::pair<ScheduleInfo, std::vector<std::shared_ptr<TaskNode>>>& tasks = m_TasksPerSchedule[schedule];
                    repeating = tasks.first.repeating->load(std::memory_order_relaxed);

                    // Tasks are running already, so this atomic add will prevent the scenario where a task is executed,
                    // decreasing the counter to 0 and falsly signalling that there are no more left when the rest have just not been enqueued
                    m_Workers.add_task_count(tasks.second.size());
                    for (auto& task : tasks.second)
                    {
                        m_Workers.enqueue(task, false);
                    }
                }
                m_Workers.wait_for_all();

                // Did repeat status changed from any of the tasks?
                for (auto& schedule : schedule_set)
                {
                    std::pair<ScheduleInfo, std::vector<std::shared_ptr<TaskNode>>>& tasks = m_TasksPerSchedule[schedule];
                    repeating = tasks.first.repeating->load(std::memory_order_relaxed);
                }

                // Keep repeating the schedule set that's registered with tskr::ExecutionPolicy::Repeat
                if (repeating)
                    schedule_set_idx -= 1;

                schedule_set_idx++;
            }

            // Have any of the schedules' repeat status changed?
            for (auto& schedule_set : m_ScheduleHashes)
            {
                for (auto& schedule : schedule_set)
                {
                    std::pair<ScheduleInfo, std::vector<std::shared_ptr<TaskNode>>>& tasks = m_TasksPerSchedule[schedule];
                    repeating = tasks.first.repeating->load(std::memory_order_relaxed);
                    if (repeating)
                        break;
                }
                if (repeating)
                    break;
            }
        }
    }

    void Tasker::halt()
    {
        m_Running.store(false);
        m_Workers.wait_for_all();
        m_Workers.stop();
    }

    Running::Running()
    {
        m_Running = std::make_shared<std::atomic_bool>(true);
    }

    void Running::stop()
    {
        m_Running->store(false, std::memory_order_relaxed);
    }
    bool Running::is_running()
    {
        return m_Running->load(std::memory_order_relaxed);
    }
}