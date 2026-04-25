# tasker
A C++ task scheduler designed for static and dynamic tasks

A high‑performance, lock‑free task scheduler built with modern C++20.  
Designed for scalability, low latency, and real‑world multithreaded workloads such as game engines, image processing, and command buffer generation.

---

## Building ⚙️
Tasker uses CMake as its build configurator. For ease of use I have added a build.bat that will generate the solution files for VS26 on Windowns. If on another platform you can run CMake with your desired generator:

```batch
mkdir build
cd build
cmake -G "Generator of choice" ../CMakeLists.txt
```
build.bat uses: "Visual Studio 18 2026" ‼️ Requires latest CMake ‼️

#### Build options:
* `-DTSKR_BUILD_EXAMPLES_ALL=1` - builds all examples in the example folder
* `-DTSKR_BUILD_EXAMPLE=simple` - builds a specific example in the example folder (in this case the simple.cpp)

## 🚀 Core Features

* [x] Can scale to the number of cores for the current system. Or explicitly choose the number of threads (workers) to spawn and their task capacity
#### Example:
```cpp
#include "tasker.hpp"
int main()
{
    uint8_t worker_count = 32;
    size_t task_capacity_per_worker = 1024;

    // Note: Default constructor parameters are std::thread::hardware_concurrency() and 256
    tskr::Tasker tasker(worker_count, task_capacity_per_worker);
}
```
---
* [x] Schedule-based execution: Tasks are grouped in user-defined schedules. The order that they are registerd in the system will be the order they will also be executed in. Schedules are executed one after the other unless specified otherwise and grouped together in a `Parallel<>` schedule set.

#### Example:
```cpp
#include "tasker.hpp"

struct Startup {};
struct Main {};
struct Render {};
struct Shutdown {};
int main()
{
    tskr::Tasker tasker;

    // Startup schedule that will execute once
    tasker.add_schedules<Startup>(tskr::ExecutionPolicy::Single)      
        // Main and Render schedules that will run in parallel on-repeat          
        .add_schedules<Parallel<Main, Render>>(tskr::ExecutionPolicy::Repeat) 
        // Shutdown schedule that will execute once
        .add_schedules<Shutdown>(tskr::ExecutionPolicy::Single);
}
```
---
* [x] Schedules can also have affinity towards a specific core/worker and/or a maximum number of cores the tasks within can run on.
‼️If max cores is greater than the mask's prefered core count, then the mask takes precedence and limits the max cores to the count of the prefered ones. ‼️

#### Example:
```cpp
#include "tasker.hpp"

struct Startup {};
struct Main {};
struct Render {};
struct Shutdown {};
int main()
{
    tskr::Tasker tasker;

    tasker.add_schedules<Startup>(tskr::ExecutionPolicy::Single)
        // Main and Render schedules that will run in parallel on-repeat on the 0th and 1st cores
        .add_schedules<Parallel<Main, Render>>(tskr::ExecutionPolicy::Repeat, 2, 0b0011) 
        .add_schedules<Shutdown>(tskr::ExecutionPolicy::Single);
}
```
---
* [x] Static Tasks - defined before execution; tied to a specific system/schedule at compile time. 
#### Example:
```cpp
#include "tasker.hpp"

struct Startup {};

void task1()
{
    // Do some work
}

void task2()
{
    // Do some more work in parallel
}

int main()
{
    tskr::Tasker tasker;

    tasker.add_schedules<Startup>(tskr::ExecutionPolicy::Single)
        // Add the two functions to the Startup schedule to be executed in parallel
        .add_tasks<Startup>(tskr::TaskFn<task1>{}, tskr::TaskFn<task2>{});

    // To run the graph simply:
    tasker.run();
    // Or chain it above to calls to add_task/add_schedule
}
```
---
* [x] Dynamic Tasks - not know in advance; can be ran at any point, by any other task in any schedule
#### Example:
```cpp
#include "tasker.hpp"

struct Startup {};

void yet_another_task() 
{
    // Even more work!
}

void task1(Commands cmds)
{
    // Do some work
    // if work is at a certain stage...

    // Using the commands parameter (which gets injected into the system), one can spawn even more tasks from running tasks.
    // ‼️Spawned tasks will respect the current schedule's affinity and max core count
    // ‼️Spawned tasks can be Standalone (not waited on for the next schedule's execution) or Scheduled (waited on)
    commands.spawn(tskr::TaskFn<yet_another_task, tskr::TaskSpawnType::Standalone>{});
}

void task2()
{
    // Do some more work in parallel
}

int main()
{
    tskr::Tasker tasker;

    tasker.add_schedules<Startup>(tskr::ExecutionPolicy::Single)
        .add_tasks<Startup>(tskr::TaskFn<task1>{}, tskr::TaskFn<task2>{});

    tasker.run();
}
```
---
* [x] Task dependencies - tasks can have two levels of scheduling: *Schedule to Schedule* (tasks defined in one shcedule set will be executed before tasks from the next schedule set); *Task to Task* (task within a schedule can be scheduled before or after other tasks).
#### Example:
```cpp
#include "tasker.hpp"

struct Startup {};
struct Shutdown {};

void task1()
{
    // Do some work
}

void task2()
{
    // Do some more work
}

int main()
{
    tskr::Tasker tasker;

    tasker.add_schedules<Startup>(tskr::ExecutionPolicy::Single)
    tasker.add_schedules<Shutdown>(tskr::ExecutionPolicy::Single)
        // task2 will be executed after task1
        .add_tasks<Startup>(tskr::TaskFn<task2>{}.after(tskr::TaskFn<task1>{}))
        // task2 will be executed before task1
        .add_tasks<Startup>(tskr::TaskFn<task2>{}.before(tskr::TaskFn<task1>{}));
    tasker.run();
}
```
---
* [x] Task sets - In addition to ordering tasks one by one, one can define a set of tasks and order it with another set of tasks
#### Example:
```cpp
#include "tasker.hpp"

struct Startup {};

void task1()
{
    // Do some work
}

void task2()
{
    // Do some more work
}

void task3()
{
    // Do some work
}

void task4()
{
    // Do some more work
}

int main()
{
    tskr::Tasker tasker;

    tasker.add_schedules<Startup>(tskr::ExecutionPolicy::Single)
        // task3 and task4 will run both in parallel before task1 and task2 are ran (also in parallel)
        .add_tasks<Startup>((tskr::TaskFn<task1>{}, tskr::TaskFn<task2>{}).after(tskr::TaskFn<task3>{}, tskr::TaskFn<task4>{})) 
    tasker.run();
}
```
---
* [x] Resources - Tasks can query for resources with their parameters. Resources can be custom types that are registered and initialized at the beginning of the program or engine-defined types (`Commands`, `Resource<ScheduleInfo>`, `Resource<Running>`, `Resource<Repeating<Schedule>>` currently, but will expand soon... TODO: add a link to a list of all and their use).

‼️Currently, tasks are not rescheduled according to resource access and mutability (but soonTM!), so it is greatly recommended to pay close attention to data access and ordering of the tasks within a schedule ‼️
#### Example:
```cpp
#include "tasker.hpp"
#include <iostream>

struct Startup {};

struct CustomResource {
    CustomResource(int _counter) : counter(_counter) {}
    int counter = 0;
}

// Registered resources can be querried through the Resource<> type
void task1(Resource<CustomResource> res)
{
    res->counter++;
}

void task2(Resource<CustomResource> res)
{
    std::cout << res->counter << std::endl;
}

int main()
{
    tskr::Tasker tasker;

    tasker.add_schedules<Startup>(tskr::ExecutionPolicy::Single)
        // Register the custom resource
        .register_resource(CustomResource(5))
        // Tasks can now querry it as Resource<CustomResource> parameter
        .add_tasks<Startup>(tskr::TaskFn<task2>{}.after(tskr::TaskFn<task1>{}))
    tasker.run();
}
```
---
* [x] Supports printing the current task graph either to the console or in a .dot format to a file with `tasker.print_graph("optional_file_path.dot")`
#### Example of the graph produced by the simple.cpp example: ![GraphViz visualization of the .dot file printed](https://github.com/blackfatcat/tasker/blob/main/graphviz.png)
---
* [ ] MT Gen of cmd bufs - WIP
Can be seen here: [Multi-threaded Command Buffer generation with DX12](https://github.com/blackfatcat/tasker/tree/main/examples/dx3d_cmds.cpp)
---
* [x] Image Processing - an example of a graph that does grayscale, blur and sobel processing on an image can be seen here: [image processing example](https://github.com/blackfatcat/tasker/tree/main/examples/image_processing.cpp). It applies grayscale then in parallel does the sobel and blur.

‼️NOTE: If you'd like to try it for yourself, you have to edit the path in the Settings resource to the full path of the image you'd like to apply the post processing to. Then it will generate 3 images either in the build folder or in the bin folder, depending on where you ran the program from.‼️
---
* [x] Utilizes lock-free task queus
    * [Correct and Efficient Work-Stealing for Weak Memory Models](https://fzn.fr/readings/ppopp13.pdf)
    * [A bounded multi-producer multi-consumer concurrent queue written in C++11](https://github.com/rigtorp/MPMCQueue)
---
* [x] Task Stealing - utilizing the Chase-Lev work-stealing alogrithm for a queue, the workers are able to steal tasks from other workers' queues, making the scheduler more efficient with the inrease of load and cores.

## ❌ Known issues

* [ ] Parallel schedules share the same resource store, making it highly inefficient to modify it from within their tasks. A possible solution could be to have a resource store per-schedule and sync them whenever desired (on a signal or a last schedule that extracts and syncs like Bevy does, I believe). For now the user has to manually sync data and make sure that there are no races between tasks by ordering them correctly.
