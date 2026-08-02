#include "rtos.hpp"
#include "scheduler.hpp"
#include "semaphore.hpp"
#include "mutex.hpp"
#include "kernel_access.hpp"

#include <utility>

namespace
{

    Scheduler kernel_scheduler;

}

namespace rtos
{
    namespace detail
    {

        Scheduler &getKernelScheduler()
        {
            return kernel_scheduler;
        }

    }

    void createTask(
        const std::string &name,
        std::uint8_t priority,
        std::function<void()> task_function)
    {
        kernel_scheduler.createTask(
            name,
            priority,
            std::move(task_function));
    }

    void start(std::uint64_t max_ticks)
    {
        kernel_scheduler.run(max_ticks);
    }

    void delay(std::uint64_t ticks)
    {
        kernel_scheduler.delayCurrentTask(ticks);
    }

    void yield()
    {
        kernel_scheduler.yieldCurrentTask();
    }

    void take(Semaphore &semaphore)
    {
        kernel_scheduler.takeSemaphore(semaphore);
    }

    bool give(Semaphore &semaphore)
    {
        return kernel_scheduler.giveSemaphore(
            semaphore);
    }

    bool lock(Mutex &mutex)
    {
        return kernel_scheduler.lockMutex(
            mutex);
    }

    bool unlock(Mutex &mutex)
    {
        return kernel_scheduler.unlockMutex(
            mutex);
    }

    void enableYieldProfiling(
        std::size_t warmup_samples,
        std::size_t expected_samples)
    {
        kernel_scheduler.enableYieldProfiling(
            warmup_samples,
            expected_samples);
    }

    const std::vector<std::uint64_t> &
    getYieldCycleSamples()
    {
        return kernel_scheduler.getYieldCycleSamples();
    }

    std::uint64_t currentTick()
    {
        return kernel_scheduler.currentTick();
    }

    void setPriorityInheritanceEnabled(bool enabled)
    {
        return kernel_scheduler.setPriorityInheritanceEnabled(enabled);
    }
    
}