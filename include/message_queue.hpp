#pragma once

#include "task.hpp"

#include <cstddef>
#include <deque>
#include <list>
#include <stdexcept>
#include <utility>

class Scheduler;

namespace rtos
{

template <typename T>
class MessageQueue
{
public:
    explicit MessageQueue(
        std::size_t capacity)
        : capacity_(capacity)
    {
        if (capacity_ == 0)
        {
            throw std::invalid_argument(
                "MessageQueue capacity must be greater than zero");
        }
    }

    MessageQueue(
        const MessageQueue &) = delete;

    MessageQueue &operator=(
        const MessageQueue &) = delete;

    std::size_t capacity() const
    {
        return capacity_;
    }

    std::size_t size() const
    {
        return buffer_.size();
    }

    bool empty() const
    {
        return buffer_.empty();
    }

    bool full() const
    {
        return buffer_.size() >=
               capacity_;
    }

private:
    struct SenderWaiter
    {
        TaskControlBlock *task =
            nullptr;

        T value;
    };

    struct ReceiverWaiter
    {
        TaskControlBlock *task =
            nullptr;

        T *output =
            nullptr;
    };

    std::size_t capacity_;

    std::deque<T> buffer_;

    std::list<SenderWaiter>
        sender_waiters_;

    std::list<ReceiverWaiter>
        receiver_waiters_;

    friend class ::Scheduler;
};

}