#include "tasker.hpp"

#include <iostream>
#include <fstream>
#include <string>

namespace tskr
{
    Tasker::Tasker(uint8_t thread_count, size_t per_worker_cap)
    {
        m_Workers = std::make_shared<WorkerPool>(thread_count, per_worker_cap);
        m_Resources = std::make_shared<ResourceStore>();

        // Insert common resources
        m_Resources->insert(Running{});
        m_Resources->insert(Commands(m_Workers, m_Resources));
        m_Resources->insert(ScheduleInfo{});
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
                    ScheduleInfo info_copy = tasks.first;

                    // FIX: This will not properly insert the schedule info for parallel schedules
                    m_Resources->insert(info_copy);

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

    namespace
    {
        std::string extract_task_name(const std::string& s)
        {
            // Find the first '('
            size_t pos = s.find('(');
            if (pos == std::string::npos)
                return {};

            // Walk left to find the start of the identifier
            size_t end = pos;
            size_t start = end;

            while (start > 0 && (std::isalnum(s[start - 1]) || s[start - 1] == '_'))
                --start;

            return s.substr(start, end - start);
        }

        std::string extract_schedule_name(std::string& s)
        {
            static constexpr std::string_view prefix = "struct ";
            s.erase(0, prefix.size());
            return s;
        }
    }

    void Tasker::print_graph(std::string_view filename)
    {
        if (filename.size() == 0)
        {
            for (auto& [key, schedule_info_and_nodes] : m_TasksPerSchedule)
            {
                std::string schedule_name_str = std::string(key);
                std::cout << "=== Schedule "<< extract_schedule_name(schedule_name_str) << " ===" << '\n';
                auto& nodes = schedule_info_and_nodes.second;

                for (auto& node : nodes) {
                    std::cout << "  Task: " << extract_task_name(node->task->name) << "\n";
                    std::cout << "    Depends on: " << node->deps_remaining << "\n";
                    std::cout << "    Dependents: ";

                    for (auto& dep : node->dependents)
                        std::cout << extract_task_name(dep->task->name) << " ";

                    std::cout << "\n\n";
                }
            }
        }
        else
        {
            std::ofstream out(filename.data(), std::ios::trunc);
            out << "digraph G {\n";
            out << "  compound=true;";
            out << "  node [shape=box, fontname=\"Consolas\"];\n";

            int prev_idx = -1;

            for (int i = 0; i < m_ScheduleHashes.size(); i++)
            {
                auto& scheduleSet = m_ScheduleHashes[i];

                out << "subgraph cluster_" << i << " {\n";
                for (auto& scheduleHash : scheduleSet)
                {
                    auto& nodes = m_TasksPerSchedule[scheduleHash].second;

                    std::string schedule_name_str = std::string(scheduleHash);
                    std::string schedule_name = extract_schedule_name(schedule_name_str);

                    out << "  subgraph cluster_" << schedule_name << " {\n";
                    out << "    label=\"" << schedule_name << "\";\n";
                    out << "    style=filled;\n";
                    out << "    color=lightgrey;\n";
                    out << "    node [style=filled, color=white];\n";

                    // Emit nodes
                    for (size_t j = 0; j < nodes.size(); j++)
                    {
                        auto& node = nodes[j];
                        std::string name = extract_task_name(node->task->name);
                        out << "      \"" << name << "\";\n";
                    }

                    // Emit edges
                    for (auto& node : nodes)
                    {
                        std::string from = extract_task_name(node->task->name);
                        for (auto& dep : node->dependents)
                        {
                            std::string to = extract_task_name(dep->task->name);
                            out << "      \"" << from << "\" -> \"" << to << "\";\n";
                        }
                    }

                    out << "    }\n"; // end schedule cluster
                }
                out << "node [style=filled, color=white];\n";
                out << "    \""<< i << "_invis\"[shape = point style = invis]\n";
                out << "}\n";

                if (prev_idx < 0)
                    out << "start -> \"" << i << "_invis\"  [XXXXXxlabel=\"\\E\", style=\"solid\" minlen=2 ltail=cluster_A1 lhead=cluster_"<< i <<" ] \n";
                else
                    out << "\"" << prev_idx << "_invis" << "\" -> \"" << i << "_invis\"  [XXXXXxlabel=\"\\E\", style=\"solid\" minlen=3 ltail=cluster_" << prev_idx << " lhead=cluster_" << i << " ] \n";

                prev_idx = i;
            }

            out << "  start [shape=Mdiamond];\n";
            out << "}\n";
        }
    }


}