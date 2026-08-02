#include "rtos.hpp"
#include "scheduler.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>

namespace
{

    double calculatePercentile(
        const std::vector<std::uint64_t>
            &sorted_samples,
        double percentile)
    {
        if (sorted_samples.empty())
        {
            return 0.0;
        }

        const double position =
            percentile *
            static_cast<double>(
                sorted_samples.size() - 1);

        const std::size_t lower_index =
            static_cast<std::size_t>(
                std::floor(position));

        const std::size_t upper_index =
            static_cast<std::size_t>(
                std::ceil(position));

        if (lower_index == upper_index)
        {
            return static_cast<double>(
                sorted_samples[lower_index]);
        }

        const double fraction =
            position -
            static_cast<double>(
                lower_index);

        const double lower_value =
            static_cast<double>(
                sorted_samples[lower_index]);

        const double upper_value =
            static_cast<double>(
                sorted_samples[upper_index]);

        return lower_value +
               fraction *
                   (upper_value -
                    lower_value);
    }

} // namespace

int main(
    int argc,
    char *argv[])
{
    if (argc != 4 && argc != 5)
    {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <tasks>"
            << " <warmup_yields_per_task>"
            << " <measured_yields_per_task>"
            << " [output_csv]\n";

        return 1;
    }

    try
    {
        const std::size_t task_count =
            std::stoull(argv[1]);

        const std::size_t
            warmup_yields_per_task =
                std::stoull(argv[2]);

        const std::size_t
            measured_yields_per_task =
                std::stoull(argv[3]);

        const std::string output_csv =
            argc == 5
                ? argv[4]
                : "yield_latency.csv";

        if (task_count == 0 ||
            measured_yields_per_task == 0)
        {
            std::cerr
                << "Task count and measured "
                << "yield count must be positive\n";

            return 1;
        }

        const std::size_t total_warmup =
            task_count *
            warmup_yields_per_task;

        const std::size_t total_measured =
            task_count *
            measured_yields_per_task;

        rtos::enableYieldProfiling(
            total_warmup,
            total_measured);

        for (std::size_t task_index = 0;
             task_index < task_count;
             ++task_index)
        {
            rtos::createTask(
                "LatencyTask" +
                    std::to_string(
                        task_index),
                1,
                [warmup_yields_per_task,
                 measured_yields_per_task]()
                {
                    const std::size_t
                        total_yields =
                            warmup_yields_per_task +
                            measured_yields_per_task;

                    for (
                        std::size_t index = 0;
                        index < total_yields;
                        ++index)
                    {
                        rtos::yield();
                    }
                });
        }

        const std::uint64_t max_ticks =
            static_cast<std::uint64_t>(
                total_warmup +
                total_measured +
                task_count +
                100);

        rtos::start(max_ticks);

        std::vector<std::uint64_t>
            samples =
                rtos::getYieldCycleSamples();

        if (samples.empty())
        {
            std::cerr
                << "No profiling samples "
                << "were collected\n";

            return 1;
        }
        /*
         * 先輸出原始執行順序。
         */
        std::ofstream csv_file(output_csv);

        if (!csv_file)
        {
            std::cerr
                << "Failed to open CSV file: "
                << output_csv
                << '\n';

            return 1;
        }

        csv_file << "sample,latency_ns\n";

        for (std::size_t index = 0;
             index < samples.size();
             ++index)
        {
            csv_file
                << index + 1
                << ','
                << samples[index]
                << '\n';
        }

        csv_file.close();

        /*
         * 排序後才能計算 percentile。
         */
        std::sort(
            samples.begin(),
            samples.end());

        const double sum =
            std::accumulate(
                samples.begin(),
                samples.end(),
                0.0);

        const double mean =
            sum /
            static_cast<double>(
                samples.size());

        const double median =
            calculatePercentile(
                samples,
                0.50);

        const double p95 =
            calculatePercentile(
                samples,
                0.95);

        const double p99 =
            calculatePercentile(
                samples,
                0.99);

        std::cout
            << std::fixed
            << std::setprecision(3);

        std::cout
            << "Tasks                 : "
            << task_count << '\n';

        std::cout
            << "Warmup yields/task    : "
            << warmup_yields_per_task
            << '\n';

        std::cout
            << "Measured yields/task  : "
            << measured_yields_per_task
            << '\n';

        std::cout
            << "Collected samples     : "
            << samples.size() << '\n';

        std::cout
            << "Mean yield cycle      : "
            << mean << " ns\n";

        std::cout
            << "Median yield cycle    : "
            << median << " ns\n";

        std::cout
            << "P95 yield cycle       : "
            << p95 << " ns\n";

        std::cout
            << "P99 yield cycle       : "
            << p99 << " ns\n";

        std::cout
            << "Minimum yield cycle   : "
            << samples.front()
            << " ns\n";

        std::cout
            << "Maximum yield cycle   : "
            << samples.back()
            << " ns\n";

        std::cout
            << "Latency CSV           : "
            << output_csv
            << '\n';
    }
    catch (const std::exception &error)
    {
        std::cerr
            << "Benchmark error: "
            << error.what()
            << '\n';

        return 1;
    }

    return 0;
}