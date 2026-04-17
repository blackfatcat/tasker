# tasker
A C++ task scheduler designed for static and dynamic tasks

A high‑performance, lock‑free task scheduler built with modern C++ (C++11–C++20).  
Designed for scalability, low latency, and real‑world multithreaded workloads such as game engines, image processing, and command buffer generation.

---

## Building ⚙️
Tasker uses CMake as its build configurator. For ease of use I have added a build.bat that will generate the solution files for VS26 on Windowns. If on another platform you can run CMake with your desired generator:

```
cmake -G "Generator of choice" CMakeLists.txt
```

#### Build options:
* `-DBUILD_EXAMPLES_ALL` - builds all example files
* `-DBUILD_EXAMPLE_NAME_OF_EXAMPLE_CPP` - builds a specific example

## 🚀 Core Features

* [ ] Can scale to the number of cores for the current system.
##### Example:
```
asd
```
---
* [ ] Supports static & dynamic tasks
    * [ ] Static Tasks - defined before execution; tied to a specific system/schedule at compile time
    * [ ] Dynamic Tasks - not know in advance; can be ran at any* point, by any* other task in any* schedule

##### Example:
```
asd
```
---
* [ ] Utilizes lock-free task queus
---
* [ ] Can specify core affinity and number of cores per task
##### Example:
```
asd
```
---
* [ ] Task dependencies
##### Example:
```
asd
```
---
* [ ] Minimal runtime overhead for CPU and memory
##### Reasoning: asd
---
* [ ] Simple Demo:
---
* [ ] Schedular can steal tasks
---
* [ ] Can print the current task graph
##### Example: [img]
---
* [ ] MT Gen of cmd bufs
##### Example: 