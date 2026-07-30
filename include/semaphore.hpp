#pragma once

#include <cstdint>
#include <list>

class Scheduler;
struct TaskControlBlock;

namespace rtos
{

class Semaphore
{
public:
    Semaphore(
        std::uint32_t initial_count,
        std::uint32_t max_count);

    Semaphore(const Semaphore &) = delete;
    Semaphore &operator=(const Semaphore &) = delete;

private:
    std::uint32_t count_;
    std::uint32_t max_count_;

    std::list<TaskControlBlock *> waiters_;

    friend class ::Scheduler;
};

void take(Semaphore &semaphore);
bool give(Semaphore &semaphore);

}