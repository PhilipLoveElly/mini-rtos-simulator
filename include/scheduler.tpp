#pragma once

#include "trace.hpp"

#include <algorithm>
#include <iostream>
#include <utility>

template <typename T>
void Scheduler::insertSenderWaiter(
    rtos::MessageQueue<T> &queue,
    TaskControlBlock *task,
    const T &value)
{
    typename rtos::MessageQueue<T>::
        SenderWaiter waiter{
            task,
            value};

    auto position =
        queue.sender_waiters_.begin();

    while (
        position !=
            queue.sender_waiters_.end() &&
        position->task
                ->effective_priority >=
            task->effective_priority)
    {
        ++position;
    }

    queue.sender_waiters_.insert(
        position,
        std::move(waiter));
}

template <typename T>
void Scheduler::insertReceiverWaiter(
    rtos::MessageQueue<T> &queue,
    TaskControlBlock *task,
    T &output)
{
    typename rtos::MessageQueue<T>::
        ReceiverWaiter waiter{
            task,
            &output};

    auto position =
        queue.receiver_waiters_.begin();

    while (
        position !=
            queue.receiver_waiters_.end() &&
        position->task
                ->effective_priority >=
            task->effective_priority)
    {
        ++position;
    }

    queue.receiver_waiters_.insert(
        position,
        waiter);
}

template <typename T>
bool Scheduler::sendMessage(
    rtos::MessageQueue<T> &queue,
    const T &value)
{
    if (current_task_ == nullptr)
    {
        return false;
    }

    /*
     * 情況一：
     * 已有 Receiver 因 Queue 空而等待。
     *
     * 不需要先放入 buffer，
     * 可以直接把資料交給 Receiver。
     */
    if (!queue.receiver_waiters_.empty())
    {
        auto receiver_waiter =
            queue.receiver_waiters_.front();

        queue.receiver_waiters_.pop_front();

        TaskControlBlock *receiver =
            receiver_waiter.task;

        if (receiver_waiter.output != nullptr)
        {
            *receiver_waiter.output =
                value;
        }

        receiver->wait_type =
            WaitType::None;

        receiver->waiting_object =
            nullptr;

        receiver->wait_result =
            true;

        enqueueReadyTask(
            receiver);

        RTOS_TRACE(
            "[Tick "
            << current_tick_
            << "] Message sent directly from "
            << current_task_->name
            << " to "
            << receiver->name
            << '\n');

        return true;
    }

    /*
     * 情況二：
     * 沒有 Receiver 等待，
     * Queue 還有空間。
     */
    if (!queue.full())
    {
        queue.buffer_.push_back(
            value);

        RTOS_TRACE(
            "[Tick "
            << current_tick_
            << "] "
            << current_task_->name
            << " sent message"
            << " (queue size = "
            << queue.buffer_.size()
            << ")\n");

        return true;
    }

    /*
     * 情況三：
     * Queue 已滿。
     *
     * Sender 必須 Blocked，
     * 並保存它準備傳送的 value。
     */

    ++queue.sender_block_count_;

    current_task_->state =
        TaskState::Blocked;

    current_task_->wait_type =
        WaitType::MessageQueueSend;

    current_task_->waiting_object =
        &queue;

    current_task_->wait_result =
        false;

    insertSenderWaiter(
        queue,
        current_task_,
        value);

    RTOS_TRACE(
        "[Tick "
        << current_tick_
        << "] "
        << current_task_->name
        << " blocked: message queue full\n");

    swapcontext(
        &current_task_->context,
        &scheduler_context_);

    return current_task_->wait_result;
}

template <typename T>
bool Scheduler::receiveMessage(
    rtos::MessageQueue<T> &queue,
    T &output)
{
    if (current_task_ == nullptr)
    {
        return false;
    }

    /*
     * 情況一：
     * Queue buffer 有資料。
     */
    if (!queue.buffer_.empty())
    {
        output =
            std::move(
                queue.buffer_.front());

        queue.buffer_.pop_front();

        RTOS_TRACE(
            "[Tick "
            << current_tick_
            << "] "
            << current_task_->name
            << " received message"
            << " (queue size = "
            << queue.buffer_.size()
            << ")\n");

        /*
         * receive 後空出了一個 slot。
         *
         * 如果有 Sender 因 Queue full 而等待，
         * 將最高優先級 Sender 的 message
         * 補進 buffer，並喚醒 Sender。
         */
        if (!queue.sender_waiters_.empty())
        {
            auto sender_waiter =
                std::move(
                    queue.sender_waiters_.front());

            queue.sender_waiters_.pop_front();

            TaskControlBlock *sender =
                sender_waiter.task;

            queue.buffer_.push_back(
                std::move(
                    sender_waiter.value));

            sender->wait_type =
                WaitType::None;

            sender->waiting_object =
                nullptr;

            sender->wait_result =
                true;

            enqueueReadyTask(
                sender);

            RTOS_TRACE(
                "[Tick "
                << current_tick_
                << "] "
                << sender->name
                << " unblocked: queue slot available\n");
        }

        return true;
    }

    /*
     * 防禦性處理：
     *
     * 一般 capacity > 0 的 Queue 中，
     * sender_waiters_ 非空通常代表 buffer 已滿，
     * 所以 buffer 空時不太會有 sender waiter。
     *
     * 但保留這個 direct handoff，
     * 可以讓狀態處理更完整。
     */
    if (!queue.sender_waiters_.empty())
    {
        auto sender_waiter =
            std::move(
                queue.sender_waiters_.front());

        queue.sender_waiters_.pop_front();

        output =
            std::move(
                sender_waiter.value);

        TaskControlBlock *sender =
            sender_waiter.task;

        sender->wait_type =
            WaitType::None;

        sender->waiting_object =
            nullptr;

        sender->wait_result =
            true;

        enqueueReadyTask(
            sender);

        RTOS_TRACE(
            "[Tick "
            << current_tick_
            << "] Message transferred directly from "
            << sender->name
            << " to "
            << current_task_->name
            << '\n');

        return true;
    }

    /*
     * 情況三：
     * Queue 完全沒有資料。
     *
     * Receiver 進入 Blocked。
     */
    ++queue.receiver_block_count_;

    current_task_->state =
        TaskState::Blocked;

    current_task_->wait_type =
        WaitType::MessageQueueReceive;

    current_task_->waiting_object =
        &queue;

    current_task_->wait_result =
        false;

    insertReceiverWaiter(
        queue,
        current_task_,
        output);

    RTOS_TRACE(
        "[Tick "
        << current_tick_
        << "] "
        << current_task_->name
        << " blocked: message queue empty\n");

    swapcontext(
        &current_task_->context,
        &scheduler_context_);

    return current_task_->wait_result;
}