#include "rtos.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>
#include <exception>
#include <string>

namespace
{

    using Clock = std::chrono::steady_clock;

    void runSchedulerBenchmark(
        std::size_t task_count,
        std::size_t yields_per_task)
    {
        if (task_count == 0 || yields_per_task == 0)
        {
            std::cerr
                << "task_count and yields_per_task must be greater than zero\n";

            return;
        }

        const std::size_t total_yields =
            task_count * yields_per_task;

        for (std::size_t task_index = 0;
             task_index < task_count;
             ++task_index)
        {
            rtos::createTask(
                "BenchmarkTask" +
                    std::to_string(task_index),
                1,
                [yields_per_task]()
                {
                    for (std::size_t iteration = 0;
                         iteration < yields_per_task;
                         ++iteration)
                    {
                        rtos::yield();
                    }
                });
        }
        const auto start_time =
            Clock::now();

        /*
         * max_ticks 必須大於所有 task 執行所需的總 tick。
         *
         * 每次 yield 通常會消耗一次 scheduler iteration，
         * 每個 task 結束也可能再消耗額外 tick。
         */
        const std::uint64_t max_ticks =
            static_cast<std::uint64_t>(
                total_yields +
                task_count +
                100);

        rtos::start(max_ticks);

        const auto end_time =
            Clock::now();

        const auto elapsed_ns =
            std::chrono::duration_cast<
                std::chrono::nanoseconds>(
                end_time - start_time)
                .count();

        const double elapsed_ms =
            static_cast<double>(elapsed_ns) /
            1'000'000.0;

        const double average_yield_ns =
            static_cast<double>(elapsed_ns) /
            static_cast<double>(total_yields);

        const double yields_per_second =
            static_cast<double>(total_yields) /
            (static_cast<double>(elapsed_ns) /
             1'000'000'000.0);

        std::cout
            << "Tasks              : "
            << task_count
            << '\n';

        std::cout
            << "Yields per task    : "
            << yields_per_task
            << '\n';

        std::cout
            << "Total yields       : "
            << total_yields
            << '\n';

        std::cout
            << std::fixed
            << std::setprecision(3);

        std::cout
            << "Elapsed time       : "
            << elapsed_ms
            << " ms\n";

        std::cout
            << "Average yield cycle cost : "
            << average_yield_ns
            << " ns\n";

        std::cout
            << "Yield throughput   : "
            << yields_per_second
            << " yields/s\n";

        std::cout
            << "----------------------------------------\n";
    }

}

int main(int argc, char *argv[])
{
    std::size_t task_count = 8;
    std::size_t yields_per_task = 10'000;

    try
    {
        if (argc >= 2)
        {
            task_count =
                static_cast<std::size_t>(
                    std::stoull(argv[1]));
        }

        if (argc >= 3)
        {
            yields_per_task =
                static_cast<std::size_t>(
                    std::stoull(argv[2]));
        }
    }
    catch (const std::exception &exception)
    {
        std::cerr
            << "Invalid argument: "
            << exception.what()
            << '\n';

        return 1;
    }

    if (task_count == 0 ||
        yields_per_task == 0)
    {
        std::cerr
            << "Arguments must be greater than zero\n";

        return 1;
    }

    runSchedulerBenchmark(
        task_count,
        yields_per_task);

    return 0;
}