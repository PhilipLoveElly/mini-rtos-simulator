#pragma once

#include "kernel_access.hpp"
#include "message_queue.hpp"
#include "scheduler.hpp"

namespace rtos
{

template <typename T>
bool send(
    MessageQueue<T> &queue,
    const T &value)
{
    Scheduler &scheduler =
        detail::getKernelScheduler();

    return scheduler.sendMessage(
        queue,
        value);
}

template <typename T>
bool receive(
    MessageQueue<T> &queue,
    T &output)
{
    Scheduler &scheduler =
        detail::getKernelScheduler();

    return scheduler.receiveMessage(
        queue,
        output);
}

}