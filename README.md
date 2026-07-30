# Mini RTOS Simulator

A C++17 mini RTOS simulator that implements core RTOS kernel scheduling and synchronization primitives using cooperative multitasking with `ucontext`.

This project demonstrates the design and implementation of fundamental RTOS components, including task scheduling, semaphores, mutexes with priority inheritance, deadlock prevention, and blocking message queues.

---

# Features

- Priority-based cooperative scheduler
- Multi-level ready queues
- Delayed task scheduling
- Counting semaphore
- Mutex with priority inheritance
- Recursive priority inheritance
- Deadlock detection and prevention
- Bounded blocking message queue
- Priority-ordered sender and receiver wait lists
- FIFO ordering for equal-priority tasks
- Direct message handoff
- C++17 implementation

---

# Project Structure

```text
mini-rtos-simulator
├── docs/
├── examples/
│   └── basic_tasks.cpp
├── include/
│   ├── kernel_access.hpp
│   ├── message_queue.hpp
│   ├── message_queue_api.hpp
│   ├── mutex.hpp
│   ├── rtos.hpp
│   ├── scheduler.hpp
│   ├── scheduler.tpp
│   ├── semaphore.hpp
│   └── task.hpp
├── src/
├── tests/
├── Makefile
└── README.md
```

---

# Architecture

```text
                  +----------------+
                  |  Application   |
                  +----------------+
                          |
                          v
                   RTOS API Layer
 createTask / delay / yield / semaphore
      mutex / message queue
                          |
                          v
                  +----------------+
                  |   Scheduler    |
                  +----------------+
                   |      |      |
                   |      |      |
                   v      v      v
            Ready Queue  Delay Queue
                   |
         +---------+----------+
         |                    |
         v                    v
    Semaphore              Mutex
                                |
                                v
                    Priority Inheritance

                   +----------------+
                   | Message Queue  |
                   +----------------+
```

---

# Synchronization Primitives

## Semaphore

- Counting semaphore
- Priority-based waiting
- FIFO ordering for equal priorities
- Direct ownership transfer

## Mutex

- Ownership tracking
- Priority inheritance
- Recursive priority inheritance
- Multiple owned mutexes
- Waiter priority reordering
- Deadlock detection
- Deadlock prevention

## Message Queue

- Fixed-capacity bounded queue
- Blocking send
- Blocking receive
- Priority-ordered sender waiters
- Priority-ordered receiver waiters
- FIFO ordering for equal priorities
- Direct handoff
- Queue refill from blocked senders

---

# Build

```bash
make
```

Run:

```bash
./mini_rtos
```

---

# Example Output

```text
HighTask waiting for mutex
Priority inheritance: LowTask priority 1 -> 3
Mutex ownership transferred to HighTask

Producer blocked
Consumer received message: 100
Producer resumed
```

---

# Current Status

## Implemented

- Scheduler
- Multi-level ready queues
- Delayed queue
- Semaphore
- Mutex
- Priority inheritance
- Deadlock prevention
- Message queue

## Planned

- Performance benchmarks
- Benchmark report
- Additional documentation

---

# License

MIT License