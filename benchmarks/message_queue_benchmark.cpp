#include "message_queue.hpp"
#include "message_queue_api.hpp"
#include "rtos.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace
{

constexpr std::uint8_t TASK_PRIORITY = 2;

bool parseSizeArgument(
    const char *value,
    std::size_t &result)
{
    try
    {
        std::size_t parsed_length = 0;

        const unsigned long long parsed =
            std::stoull(
                value,
                &parsed_length);

        if (value[parsed_length] != '\0')
        {
            return false;
        }

        if (parsed >
            std::numeric_limits<std::size_t>::max())
        {
            return false;
        }

        result =
            static_cast<std::size_t>(
                parsed);

        return true;
    }
    catch (...)
    {
        return false;
    }
}

} // namespace

int main(
    int argc,
    char *argv[])
{
    /*
     * Usage:
     *
     * ./message_queue_benchmark \
     *     <capacity> \
     *     <message_count>
     *
     * Example:
     *
     * ./message_queue_benchmark 16 100000
     */
    if (argc != 3)
    {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <capacity> <message_count>\n";

        return EXIT_FAILURE;
    }

    std::size_t capacity = 0;
    std::size_t message_count = 0;

    if (!parseSizeArgument(
            argv[1],
            capacity) ||
        capacity == 0)
    {
        std::cerr
            << "Invalid capacity: "
            << argv[1]
            << '\n';

        return EXIT_FAILURE;
    }

    if (!parseSizeArgument(
            argv[2],
            message_count) ||
        message_count == 0)
    {
        std::cerr
            << "Invalid message_count: "
            << argv[2]
            << '\n';

        return EXIT_FAILURE;
    }

    rtos::MessageQueue<std::uint64_t>
        message_queue(capacity);

    std::size_t produced_messages = 0;
    std::size_t consumed_messages = 0;

    std::uint64_t checksum = 0;

    /*
     * Producer 與 Consumer 使用相同優先級。
     *
     * 這樣不會因固定優先級差異，讓其中一方永久壓制
     * 另一方；同優先級 Task 依 ready queue 的 FIFO
     * 規則執行。
     */
    rtos::createTask(
        "Producer",
        TASK_PRIORITY,
        [&]()
        {
            for (std::size_t index = 0;
                 index < message_count;
                 ++index)
            {
                const std::uint64_t message =
                    static_cast<std::uint64_t>(
                        index);

                const bool sent =
                    rtos::send(
                        message_queue,
                        message);

                if (!sent)
                {
                    return;
                }

                ++produced_messages;
            }
        });

    rtos::createTask(
        "Consumer",
        TASK_PRIORITY,
        [&]()
        {
            for (std::size_t index = 0;
                 index < message_count;
                 ++index)
            {
                std::uint64_t message = 0;

                const bool received =
                    rtos::receive(
                        message_queue,
                        message);

                if (!received)
                {
                    return;
                }

                checksum +=
                    message;

                ++consumed_messages;
            }
        });

    /*
     * Task 建立完成後才開始計時，
     * 避免把 createTask() 成本算進 queue throughput。
     */
    const auto start_time =
        std::chrono::steady_clock::now();

    /*
     * 上限必須足夠完成 producer/consumer。
     *
     * 每個 message 最壞可能伴隨一次或多次 scheduler
     * iteration，因此先使用較寬鬆上限。
     */
    const std::uint64_t max_ticks =
        static_cast<std::uint64_t>(
            message_count) *
        4ULL +
        100ULL;

    rtos::start(
        max_ticks);

    const auto end_time =
        std::chrono::steady_clock::now();

    if (produced_messages !=
            message_count ||
        consumed_messages !=
            message_count)
    {
        std::cerr
            << "Benchmark did not complete\n"
            << "Produced: "
            << produced_messages
            << '\n'
            << "Consumed: "
            << consumed_messages
            << '\n';

        return EXIT_FAILURE;
    }

    const auto elapsed_ns =
        std::chrono::duration_cast<
            std::chrono::nanoseconds>(
                end_time -
                start_time)
            .count();

    if (elapsed_ns <= 0)
    {
        std::cerr
            << "Invalid elapsed time\n";

        return EXIT_FAILURE;
    }

    const double elapsed_seconds =
        static_cast<double>(
            elapsed_ns) /
        1'000'000'000.0;

    const double throughput =
        static_cast<double>(
            consumed_messages) /
        elapsed_seconds;

    /*
     * CSV output:
     *
     * capacity,
     * messages,
     * elapsed_ns,
     * throughput,
     * producer_blocks,
     * consumer_blocks,
     * checksum
     */
    std::cout
        << capacity
        << ','
        << message_count
        << ','
        << elapsed_ns
        << ','
        << throughput
        << ','
        << message_queue.senderBlockCount()
        << ','
        << message_queue.receiverBlockCount()
        << ','
        << checksum
        << '\n';

    return EXIT_SUCCESS;
}