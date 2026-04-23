#include "tasker.hpp"

namespace tskr
{
    Tasker::Tasker(uint8_t thread_count, size_t per_worker_cap)
    {
        m_Workers = std::make_shared<WorkerPool>(thread_count, per_worker_cap);
        m_Resources = std::make_shared<ResourceStore>();
        m_Resources->insert(Running{});
        m_Resources->insert(Commands(m_Workers, m_Resources));
    }

    Tasker::~Tasker()
    {
        halt();
    }

	void Tasker::run()
    {
        // TODO: Pass in resources
        m_Workers->work();

        bool repeating = true;

        // Main loop
        while (m_Resources->get<Running>()->is_running() && repeating)
        {
            repeating = false;
            //for (auto& schedule_set : m_ScheduleHashes)
            for (size_t schedule_set_idx = 0; schedule_set_idx < m_ScheduleHashes.size();)
            {
                std::vector<KEY_TYPE>& schedule_set = m_ScheduleHashes[schedule_set_idx];

                for (auto& schedule : schedule_set)
                {
                    std::pair<ScheduleInfo, std::vector<std::shared_ptr<TaskNode>>>& tasks = m_TasksPerSchedule[schedule];
                    repeating = tasks.first.repeating->load(std::memory_order_relaxed);

                    // Tasks are running already, so this atomic add will prevent the scenario where a task is executed,
                    // decreasing the counter to 0 and falsly signalling that there are no more left when the rest have just not been enqueued
                    m_Workers->add_task_count(tasks.second.size());
                    for (auto& task : tasks.second)
                    {
                        m_Workers->enqueue(task, false);
                    }
                }
                m_Workers->wait_for_all();

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
        m_Workers->wait_for_all();
        m_Workers->stop();
    }
}