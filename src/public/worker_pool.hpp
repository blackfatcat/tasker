#include <vector>
#include <atomic>
#include <thread>
#include <cstdint>

namespace tskr
{
    class WorkerPool
    {
    private:
        std::vector<std::thread> m_Workers;
        std::atomic<bool> m_Shutdown{ false };
        uint8_t m_ThreadCount = 0;
    public:
        // TODO: Add affinity mask
        WorkerPool(uint8_t thread_count = std::thread::hardware_concurrency());
        ~WorkerPool();

        void work();
        void stop();

    private:
        void worker_loop(int worker_id);
    };
    
} // namespace tskr
