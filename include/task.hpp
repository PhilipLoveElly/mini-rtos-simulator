#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <string>
#include <ucontext.h>
#include <vector>

namespace rtos
{
    class Mutex;
}

enum class TaskState
{
    Ready,
    Running,
    Blocked,
    Terminated
};

enum class WaitType
{
    None,
    Delay,
    Semaphore,
    Mutex,
    MessageQueueSend,
    MessageQueueReceive
};

struct TaskControlBlock
{
    std::uint32_t id = 0;

    std::string name;

    /*
     * Task 建立時指定的原始優先級。
     * Priority Inheritance 不會修改它。
     */
    std::uint8_t base_priority = 0;

    /*
     * Scheduler 真正使用的優先級。
     *
     * 一般情況：
     * effective_priority == base_priority
     *
     * 發生 Priority Inheritance：
     * effective_priority > base_priority
     */
    std::uint8_t effective_priority = 0;

    TaskState state =
        TaskState::Ready;

    std::uint64_t wake_tick = 0;

    std::function<void()> task_function;

    ucontext_t context{};

    std::vector<char> stack;

    /*
     * 此 Task 目前持有的 Mutex。
     *
     * 不能只記一把，因為實際 Task
     * 可能同時持有多把 Mutex。
     */
    std::list<rtos::Mutex *> owned_mutexes;

    /*
     * 如果此 Task 正在等待 Mutex，
     * 指向那一把 Mutex。
     *
     * 沒有等待時為 nullptr。
     */
    rtos::Mutex *waiting_mutex = nullptr;

    WaitType wait_type =
        WaitType::None;

    void *waiting_object =
        nullptr;

    bool wait_result =
        false;
};