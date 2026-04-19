#include "tasker.hpp"

namespace tskr
{
    Tasker::Tasker(/* args */)
    {
    }

    Tasker::~Tasker()
    {
    }

	void Tasker::run()
    {
        // TODO: Pass in resources
        workers.work();
    }

    void Tasker::halt()
    {
        workers.stop();
    }
}