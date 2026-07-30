#include "scheduler.hpp"
#include "semaphore.hpp"
#include "mutex.hpp"

#include <algorithm>
#include <iostream>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <unordered_set>
#include <vector>

Scheduler *Scheduler::active_scheduler_ = nullptr;

void Scheduler::createTask(
    const std::string &name,
    std::uint8_t priority,
    std::function<void()> task_function)
{
    auto task = std::make_unique<TaskControlBlock>();

    task->id = next_task_id_++;
    task->name = name;
    task->base_priority = priority;
    task->effective_priority = priority;
    task->state = TaskState::Ready;
    task->wake_tick = 0;
    task->task_function = std::move(task_function);

    task->stack.resize(TASK_STACK_SIZE);

    getcontext(&task->context);

    task->context.uc_stack.ss_sp =
        task->stack.data();

    task->context.uc_stack.ss_size =
        task->stack.size();

    task->context.uc_link =
        &scheduler_context_;

    makecontext(
        &task->context,
        &Scheduler::taskEntry,
        0);

    TaskControlBlock *created_task =
        task.get();

    tasks_.push_back(std::move(task));

    enqueueReadyTask(created_task);
}

void Scheduler::taskEntry()
{
    Scheduler *scheduler = active_scheduler_;

    if (scheduler == nullptr ||
        scheduler->current_task_ == nullptr)
    {
        return;
    }

    TaskControlBlock *task =
        scheduler->current_task_;

    task->task_function();

    task->state = TaskState::Terminated;

    std::cout
        << "[Tick "
        << scheduler->current_tick_
        << "] Task terminated: "
        << task->name
        << '\n';
}

void Scheduler::updateBlockedTasks()
{
    while (!delayed_queue_.empty())
    {
        TaskControlBlock *task =
            delayed_queue_.front();

        if (task->wake_tick > current_tick_)
        {
            break;
        }

        std::cout
            << "[Tick "
            << current_tick_
            << "] Task ready: "
            << task->name
            << '\n';

        delayed_queue_.pop_front();

        enqueueReadyTask(task);
    }
}

void Scheduler::enqueueReadyTask(
    TaskControlBlock *task)
{
    if (task == nullptr)
    {
        return;
    }

    if (task->effective_priority >= MAX_PRIORITIES)
    {
        throw std::out_of_range(
            "Task priority exceeds MAX_PRIORITIES");
    }

    task->state = TaskState::Ready;

    ready_queues_[task->effective_priority].push_back(task);
}

TaskControlBlock *Scheduler::selectNextTask()
{
    for (std::size_t priority = MAX_PRIORITIES;
         priority > 0;
         --priority)
    {
        auto &queue =
            ready_queues_[priority - 1];

        if (queue.empty())
        {
            continue;
        }

        TaskControlBlock *task =
            queue.front();

        queue.pop_front();

        return task;
    }

    return nullptr;
}

void Scheduler::run(std::uint64_t max_ticks)
{
    active_scheduler_ = this;

    while (current_tick_ < max_ticks && hasActiveTasks())
    {
        updateBlockedTasks();

        TaskControlBlock *next_task =
            selectNextTask();

        if (next_task == nullptr)
        {
            if (!advanceToNextWakeTick(max_ticks))
            {
                break;
            }

            continue;
        }

        current_task_ = next_task;
        current_task_->state = TaskState::Running;

        std::cout
            << "[Tick "
            << current_tick_
            << "] Running: "
            << current_task_->name
            << '\n';

        swapcontext(
            &scheduler_context_,
            &current_task_->context);

        if (current_task_->state == TaskState::Running)
        {
            enqueueReadyTask(current_task_);
        }

        ++current_tick_;
    }

    if (!hasActiveTasks())
    {
        std::cout
            << "[Scheduler] All tasks terminated at tick "
            << current_tick_
            << '\n';
    }
    else
    {
        std::cout
            << "[Scheduler] Maximum tick limit reached: "
            << max_ticks
            << '\n';
    }

    current_task_ = nullptr;
    active_scheduler_ = nullptr;
}

void Scheduler::insertDelayedTask(
    TaskControlBlock *task)
{

    auto position = delayed_queue_.begin();

    while (position != delayed_queue_.end() &&
           (*position)->wake_tick <= task->wake_tick)
    {
        ++position;
    }

    delayed_queue_.insert(position, task);
}

void Scheduler::delayCurrentTask(std::uint64_t ticks)
{
    if (current_task_ == nullptr)
    {
        return;
    }

    current_task_->wake_tick =
        current_tick_ + ticks;

    current_task_->state =
        TaskState::Blocked;

    insertDelayedTask(current_task_);

    std::cout
        << "[Tick "
        << current_tick_
        << "] "
        << current_task_->name
        << " blocked until tick "
        << current_task_->wake_tick
        << '\n';

    swapcontext(
        &current_task_->context,
        &scheduler_context_);
}

bool Scheduler::hasActiveTasks() const
{
    for (const auto &task : tasks_)
    {
        if (task->state != TaskState::Terminated)
        {
            return true;
        }
    }

    return false;
}

void Scheduler::yieldCurrentTask()
{
    if (current_task_ == nullptr)
    {
        return;
    }

    std::cout
        << "[Tick "
        << current_tick_
        << "] "
        << current_task_->name
        << " yielded\n";

    enqueueReadyTask(current_task_);

    swapcontext(
        &current_task_->context,
        &scheduler_context_);
}

bool Scheduler::advanceToNextWakeTick(
    std::uint64_t max_ticks)
{
    if (delayed_queue_.empty())
    {
        return false;
    }

    const std::uint64_t next_wake_tick =
        delayed_queue_.front()->wake_tick;

    if (next_wake_tick >= max_ticks)
    {
        current_tick_ = max_ticks;
        return false;
    }

    if (next_wake_tick > current_tick_)
    {
        std::cout
            << "[Tick "
            << current_tick_
            << "] No ready task, advancing to tick "
            << next_wake_tick
            << '\n';

        current_tick_ = next_wake_tick;
    }

    return true;
}

void Scheduler::insertSemaphoreWaiter(
    rtos::Semaphore &semaphore,
    TaskControlBlock *task)
{
    auto position =
        semaphore.waiters_.begin();

    while (position != semaphore.waiters_.end() &&
           (*position)->effective_priority >= task->effective_priority)
    {
        ++position;
    }

    semaphore.waiters_.insert(
        position,
        task);
}

void Scheduler::insertMutexWaiter(
    rtos::Mutex &mutex,
    TaskControlBlock *task)
{
    auto position =
        mutex.waiters_.begin();

    while (position != mutex.waiters_.end() &&
           (*position)->effective_priority >= task->effective_priority)
    {
        ++position;
    }

    mutex.waiters_.insert(
        position,
        task);
}

void Scheduler::reorderMutexWaiter(
    rtos::Mutex &mutex,
    TaskControlBlock *task)
{
    if (task == nullptr)
    {
        return;
    }

    auto position =
        std::find(
            mutex.waiters_.begin(),
            mutex.waiters_.end(),
            task);

    if (position ==
        mutex.waiters_.end())
    {
        return;
    }

    /*
     * 先用舊位置移除。
     */
    mutex.waiters_.erase(
        position);

    /*
     * 再根據目前新的 effective_priority
     * 重新插入。
     */
    insertMutexWaiter(
        mutex,
        task);
}

void Scheduler::takeSemaphore(
    rtos::Semaphore &semaphore)
{
    if (current_task_ == nullptr)
    {
        return;
    }

    if (semaphore.count_ > 0)
    {
        --semaphore.count_;
        return;
    }

    current_task_->state =
        TaskState::Blocked;

    insertSemaphoreWaiter(
        semaphore,
        current_task_);

    std::cout
        << "[Tick "
        << current_tick_
        << "] "
        << current_task_->name
        << " waiting for semaphore\n";

    swapcontext(
        &current_task_->context,
        &scheduler_context_);
}

bool Scheduler::giveSemaphore(
    rtos::Semaphore &semaphore)
{
    if (!semaphore.waiters_.empty())
    {
        TaskControlBlock *task =
            semaphore.waiters_.front();

        semaphore.waiters_.pop_front();

        enqueueReadyTask(task);

        std::cout
            << "[Tick "
            << current_tick_
            << "] Semaphore woke: "
            << task->name
            << '\n';

        return true;
    }

    if (semaphore.count_ >=
        semaphore.max_count_)
    {
        std::cout
            << "[Tick "
            << current_tick_
            << "] Semaphore give failed: full\n";

        return false;
    }

    ++semaphore.count_;

    return true;
}

bool Scheduler::lockMutex(
    rtos::Mutex &mutex)
{
    if (current_task_ == nullptr)
    {
        return false;
    }

    /*
     * Mutex 沒有 owner：
     * current task 直接取得 ownership。
     */
    if (mutex.owner_ == nullptr)
    {
        mutex.owner_ =
            current_task_;

        current_task_
            ->owned_mutexes
            .push_back(&mutex);

        std::cout
            << "[Tick "
            << current_tick_
            << "] "
            << current_task_->name
            << " acquired mutex\n";

        return true;
    }

    /*
     * 目前使用 non-recursive Mutex。
     */
    if (mutex.owner_ == current_task_)
    {
        std::cout
            << "[Tick "
            << current_tick_
            << "] "
            << current_task_->name
            << " attempted recursive mutex lock\n";

        return false;
    }

    TaskControlBlock *owner =
        mutex.owner_;

    /*
     * 在真正阻塞之前檢查：
     * 此次 lock 是否會形成等待環。
     */
    std::vector<TaskControlBlock *>
        deadlock_path;

    if (wouldCreateDeadlock(
            current_task_,
            mutex,
            deadlock_path))
    {
        std::cout
            << "[Tick "
            << current_tick_
            << "] Deadlock prevented: ";

        printDeadlockPath(
            deadlock_path);

        std::cout
            << '\n';

        return false;
    }

    /*
     * 確認不會形成 deadlock 後，
     * 才把 Task 轉為 Blocked。
     */
    current_task_->state =
        TaskState::Blocked;

    current_task_->waiting_mutex =
        &mutex;

    insertMutexWaiter(
        mutex,
        current_task_);

    std::cout
        << "[Tick "
        << current_tick_
        << "] "
        << current_task_->name
        << " waiting for mutex owned by "
        << owner->name
        << '\n';

    /*
     * 重新計算 owner 的有效優先級。
     *
     * 若 owner 自己也正在等待另一把 Mutex，
     * recomputeEffectivePriority() 會沿著
     * waiting_mutex->owner_ 繼續向上傳遞。
     */
    recomputeEffectivePriority(
        owner);

    swapcontext(
        &current_task_->context,
        &scheduler_context_);

    /*
     * Task 被喚醒時，unlockMutex()
     * 已經直接將 Mutex ownership 交給它。
     */
    return mutex.owner_ ==
           current_task_;
}

bool Scheduler::unlockMutex(
    rtos::Mutex &mutex)
{
    if (current_task_ == nullptr)
    {
        return false;
    }

    /*
     * 只有 owner 可以 unlock。
     */
    if (mutex.owner_ != current_task_)
    {
        std::cout
            << "[Tick "
            << current_tick_
            << "] Mutex unlock failed: "
            << current_task_->name
            << " is not the owner\n";

        return false;
    }

    TaskControlBlock *old_owner =
        current_task_;

    /*
     * 此 Mutex 不再由 old_owner 持有。
     */
    old_owner
        ->owned_mutexes
        .remove(&mutex);

    /*
     * 沒有 waiter：
     * Mutex 直接變成 unlocked。
     */
    if (mutex.waiters_.empty())
    {
        mutex.owner_ =
            nullptr;

        std::cout
            << "[Tick "
            << current_tick_
            << "] "
            << old_owner->name
            << " released mutex\n";

        /*
         * 釋放 Mutex 後，重新計算 owner
         * 還需要保留多少有效優先級。
         */
        recomputeEffectivePriority(
            old_owner);

        return true;
    }

    /*
     * 取最高有效優先級 waiter。
     */
    TaskControlBlock *next_owner =
        mutex.waiters_.front();

    mutex.waiters_.pop_front();

    /*
     * Direct ownership handoff。
     */
    mutex.owner_ =
        next_owner;

    next_owner->waiting_mutex =
        nullptr;

    next_owner
        ->owned_mutexes
        .push_back(&mutex);

    enqueueReadyTask(
        next_owner);

    std::cout
        << "[Tick "
        << current_tick_
        << "] Mutex ownership transferred from "
        << old_owner->name
        << " to "
        << next_owner->name
        << '\n';

    /*
     * old_owner 已釋放此 Mutex，
     * 重新計算它的有效優先級。
     */
    recomputeEffectivePriority(
        old_owner);

    return true;
}

void Scheduler::removeReadyTask(
    TaskControlBlock *task)
{
    for (auto &queue : ready_queues_)
    {
        auto position =
            std::find(
                queue.begin(),
                queue.end(),
                task);

        if (position != queue.end())
        {
            queue.erase(position);
            return;
        }
    }
}

void Scheduler::setEffectivePriority(
    TaskControlBlock *task,
    std::uint8_t new_priority)
{
    if (task == nullptr)
    {
        return;
    }

    if (task->effective_priority ==
        new_priority)
    {
        return;
    }

    const bool was_ready =
        task->state == TaskState::Ready;

    /*
     * Task 若已經位於 Ready Queue，
     * 必須先從舊 Queue 移除。
     */
    if (was_ready)
    {
        removeReadyTask(task);
    }

    task->effective_priority =
        new_priority;

    /*
     * 再依照新的有效優先級，
     * 放回正確的 Ready Queue。
     */
    if (was_ready)
    {
        enqueueReadyTask(task);
    }
}

void Scheduler::recomputeEffectivePriority(
    TaskControlBlock *task)
{
    std::unordered_set<TaskControlBlock *> visited;

    recomputeEffectivePriorityRecursive(
        task,
        visited);
}

void Scheduler::recomputeEffectivePriorityRecursive(
    TaskControlBlock *task,
    std::unordered_set<TaskControlBlock *> &visited)
{
    if (task == nullptr)
    {
        return;
    }

    /*
     * 防止等待關係形成環時無限遞迴。
     */
    if (!visited.insert(task).second)
    {
        return;
    }

    std::uint8_t new_priority =
        task->base_priority;

    /*
     * 檢查 task 仍持有的所有 Mutex。
     */
    for (rtos::Mutex *owned_mutex :
         task->owned_mutexes)
    {
        if (owned_mutex == nullptr ||
            owned_mutex->waiters_.empty())
        {
            continue;
        }

        TaskControlBlock *highest_waiter =
            owned_mutex->waiters_.front();

        if (highest_waiter == nullptr)
        {
            continue;
        }

        new_priority =
            std::max(
                new_priority,
                highest_waiter
                    ->effective_priority);
    }

    const std::uint8_t old_priority =
        task->effective_priority;

    setEffectivePriority(
        task,
        new_priority);

    /*
     * task 若正在等待另一把 Mutex，
     * priority 改變後要重排 waiter 位置。
     */
    if (old_priority != new_priority &&
        task->waiting_mutex != nullptr)
    {
        reorderMutexWaiter(
            *task->waiting_mutex,
            task);
    }

    if (old_priority != new_priority)
    {
        if (new_priority > old_priority)
        {
            std::cout
                << "[Tick "
                << current_tick_
                << "] Priority inheritance: "
                << task->name
                << " priority "
                << static_cast<int>(old_priority)
                << " -> "
                << static_cast<int>(new_priority)
                << '\n';
        }
        else
        {
            std::cout
                << "[Tick "
                << current_tick_
                << "] Priority recomputed: "
                << task->name
                << " priority "
                << static_cast<int>(old_priority)
                << " -> "
                << static_cast<int>(new_priority)
                << '\n';
        }
    }

    /*
     * 若 task 自己也正在等待 Mutex，
     * 代表可能存在 priority inheritance chain。
     */
    if (task->waiting_mutex == nullptr)
    {
        return; 
    }

    rtos::Mutex *waiting_mutex =
        task->waiting_mutex;

    TaskControlBlock *upstream_owner =
        waiting_mutex->owner_;

    if (upstream_owner == nullptr ||
        upstream_owner == task)
    {
        return;
    }

    /*
     * 繼續更新上游 owner。
     */
    recomputeEffectivePriorityRecursive(
        upstream_owner,
        visited);
}

bool Scheduler::wouldCreateDeadlock(
    TaskControlBlock *requesting_task,
    rtos::Mutex &requested_mutex,
    std::vector<TaskControlBlock *> &deadlock_path) const
{
    /*
     * 確保呼叫者傳進來的 vector
     * 不包含上次偵測留下的資料。
     */
    deadlock_path.clear();

    if (requesting_task == nullptr)
    {
        return false;
    }

    /*
     * 這次準備新增的等待關係是：
     *
     * requesting_task
     *      ↓
     * requested_mutex.owner_
     *
     * 所以路徑第一個節點先放 requesting_task。
     */
    deadlock_path.push_back(
        requesting_task);

    TaskControlBlock *current_owner =
        requested_mutex.owner_;

    std::unordered_set<TaskControlBlock *> visited;

    while (current_owner != nullptr)
    {
        /*
         * 將目前追蹤到的 Mutex owner
         * 加入路徑。
         */
        deadlock_path.push_back(
            current_owner);

        /*
         * 如果回到 requesting_task，
         * 完整 cycle 已經形成。
         *
         * 例如：
         *
         * TaskC -> TaskA -> TaskB -> TaskC
         */
        if (current_owner ==
            requesting_task)
        {
            return true;
        }

        /*
         * 如果 current_owner 之前已經走過，
         * 代表原本的等待鏈中已經存在 cycle。
         */
        if (!visited.insert(
                        current_owner)
                 .second)
        {
            return true;
        }

        /*
         * current_owner 沒有等待其他 Mutex，
         * 代表等待鏈在這裡終止。
         */
        if (current_owner->waiting_mutex ==
            nullptr)
        {
            deadlock_path.clear();

            return false;
        }

        /*
         * 沿著：
         *
         * Task
         *  -> waiting_mutex
         *  -> owner
         *
         * 繼續追蹤下一個 Task。
         */
        current_owner =
            current_owner
                ->waiting_mutex
                ->owner_;
    }

    /*
     * owner 鏈斷掉，沒有形成 cycle。
     */
    deadlock_path.clear();

    return false;
}

void Scheduler::printDeadlockPath(
    const std::vector<TaskControlBlock *> &path) const
{
    for (std::size_t index = 0;
         index < path.size();
         ++index)
    {
        if (path[index] != nullptr)
        {
            std::cout
                << path[index]->name;
        }
        else
        {
            std::cout
                << "<null>";
        }

        if (index + 1 <
            path.size())
        {
            std::cout
                << " -> ";
        }
    }
}
