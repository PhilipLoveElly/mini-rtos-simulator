#pragma once

#include <list>

class Scheduler;
struct TaskControlBlock;

namespace rtos
{

class Mutex
{
public:
    Mutex() = default;

    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

private:
    TaskControlBlock *owner_ = nullptr;

    std::list<TaskControlBlock *> waiters_;

    friend class ::Scheduler;
};

bool lock(Mutex &mutex);

bool unlock(Mutex &mutex);

} // namespace rtos