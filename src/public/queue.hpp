#include <atomic>
#include <vector>
#include <memory>

#include "task.hpp"

namespace tskr
{
    // Push and Pop to and from the back. Steal from the front. Chase-Lev style work-stealing double ended queue. Efficient for thread local queues.
    // The algorithm holds only if push and pop are performed by the same thread.
    // Other threads can only call steal on the queue.
    class WorkStealingDeque
    {
    private:
        size_t m_Cap; 
        std::vector<std::unique_ptr<std::atomic<Task*>>> m_TaskBuf;
        std::atomic<size_t> m_Top;
        std::atomic<size_t> m_Bottom;
    public:
        WorkStealingDeque(size_t cap = 1024);
        ~WorkStealingDeque();

        WorkStealingDeque(const WorkStealingDeque&) = delete;
        WorkStealingDeque& operator=(const WorkStealingDeque&) = delete;
        WorkStealingDeque(WorkStealingDeque&&) = delete;
        WorkStealingDeque& operator=(WorkStealingDeque&&) = delete;

        /// @brief
        /// Pushes a task to the back of the queue, doubling its size if no space is left
        /// @param task Task to be pushed
        void push(Task* task);

        /// @brief
        /// Tries to push a task to the back of the queue, without increasing the size if no space is left
        /// @param task Task to be pushed
        /// @return `true` if task was successfully pushed, `false` otherwise
        bool try_push(Task* task);

        /// @brief
        /// Tries to retrieve a task from the front of the queue
        /// @param task Reference to a task that will be filled up, if the operation succeeds
        /// @return `true` if task was successfully retrieved, `false` otherwise
        bool try_pop(Task* task);

        /// @brief
        /// Tries to retrieve a task from the front of the queue
        /// @param task Reference to a task that will be filled up, if the operation succeeds
        /// @return `true` if task was successfully retrieved, `false` otherwise
        bool try_steal(Task* task);
    };
} // namespace tskr
