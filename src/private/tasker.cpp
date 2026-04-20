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
    }

    void Tasker::halt()
    {
        workers.wait_for_all();
        workers.stop();
    }
}