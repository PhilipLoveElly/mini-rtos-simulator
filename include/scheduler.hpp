#pragma once

#include "task.hpp"
#include "message_queue.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <ucontext.h>
#include <vector>
#include <memory>
#include <array>
#include <deque>
#include <list>
#include <unordered_set>

namespace rtos
{

    class Semaphore;
    class Mutex;

}

class Scheduler
{
public:
    Scheduler() = default;

    void createTask(
        const std::string &name,
        std::uint8_t priority,
        std::function<void()> task_function);

    void run(std::uint64_t max_ticks);

    void delayCurrentTask(std::uint64_t ticks);

    void yieldCurrentTask();

    void takeSemaphore(
        rtos::Semaphore &semaphore);

    bool giveSemaphore(
        rtos::Semaphore &semaphore);

    bool lockMutex(
        rtos::Mutex &mutex);

    bool unlockMutex(
        rtos::Mutex &mutex);

    template <typename T>
    bool sendMessage(
        rtos::MessageQueue<T> &queue,
        const T &value);

    template <typename T>
    bool receiveMessage(
        rtos::MessageQueue<T> &queue,
        T &output);

private:
    static constexpr std::size_t TASK_STACK_SIZE = 64 * 1024;

    static constexpr std::size_t MAX_PRIORITIES = 8;

    static void taskEntry();

    TaskControlBlock *selectNextTask();

    void enqueueReadyTask(TaskControlBlock *task);

    void insertDelayedTask(TaskControlBlock *task);

    void updateBlockedTasks();

    bool hasActiveTasks() const;

    std::array<
        std::deque<TaskControlBlock *>,
        MAX_PRIORITIES>
        ready_queues_;

    std::list<TaskControlBlock *> delayed_queue_;

    std::vector<std::unique_ptr<TaskControlBlock>> tasks_;

    ucontext_t scheduler_context_{};

    std::uint64_t current_tick_ = 0;
    std::uint32_t next_task_id_ = 0;

    TaskControlBlock *current_task_ = nullptr;

    static Scheduler *active_scheduler_;

    bool advanceToNextWakeTick(std::uint64_t max_ticks);

    void insertSemaphoreWaiter(
        rtos::Semaphore &semaphore,
        TaskControlBlock *task);

    void insertMutexWaiter(
        rtos::Mutex &mutex,
        TaskControlBlock *task);

    // Priority Inheritance helpers

    void removeReadyTask(
        TaskControlBlock *task);

    void setEffectivePriority(
        TaskControlBlock *task,
        std::uint8_t new_priority);

    void recomputeEffectivePriority(
        TaskControlBlock *task);

    void reorderMutexWaiter(
        rtos::Mutex &mutex,
        TaskControlBlock *task);

    void recomputeEffectivePriorityRecursive(
        TaskControlBlock *task,
        std::unordered_set<TaskControlBlock *> &visited);

    bool wouldCreateDeadlock(
        TaskControlBlock *requesting_task,
        rtos::Mutex &requested_mutex,
        std::vector<TaskControlBlock *> &deadlock_path) const;

    void printDeadlockPath(
        const std::vector<TaskControlBlock *> &path) const;

    template <typename T>
    void insertSenderWaiter(
        rtos::MessageQueue<T> &queue,
        TaskControlBlock *task,
        const T &value);

    template <typename T>
    void insertReceiverWaiter(
        rtos::MessageQueue<T> &queue,
        TaskControlBlock *task,
        T &output);
};

#include "scheduler.tpp"