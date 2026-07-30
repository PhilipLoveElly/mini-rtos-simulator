#pragma once

#include "message_queue.hpp"
#include "message_queue_api.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace rtos
{
    class Semaphore;
    class Mutex;

    void take(
        Semaphore &semaphore);

    bool give(
        Semaphore &semaphore);

    bool lock(Mutex &mutex);

    bool unlock(Mutex &mutex);

    void createTask(
        const std::string &name,
        std::uint8_t priority,
        std::function<void()> task_function);

    void start(std::uint64_t max_ticks);

    void delay(std::uint64_t ticks);

    void yield();

}