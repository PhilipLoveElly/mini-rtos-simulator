#include "mutex.hpp"
#include "rtos.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{

constexpr std::uint8_t HIGH_PRIORITY = 3;
constexpr std::uint8_t MEDIUM_PRIORITY = 2;
constexpr std::uint8_t LOW_PRIORITY = 1;

/*
 * LowTask 在 critical section 中執行幾次 yield。
 *
 * 這個值固定，讓 benchmark 主要觀察
 * MediumTask workload 對 HighTask 等待時間的影響。
 */
constexpr std::size_t LOW_CRITICAL_ITERATIONS = 5;

/*
 * HighTask 與 MediumTask 的 delay 時間：
 *
 * Tick 0: HighTask 執行並 delay(3)
 * Tick 1: MediumTask 執行並 delay(2)
 * Tick 2: LowTask 執行並取得 mutex
 * Tick 3: HighTask 與 MediumTask ready
 *
 * HighTask 優先級較高，因此先嘗試 lock，
 * 接著因 mutex 被 LowTask 持有而阻塞。
 */
constexpr std::uint64_t HIGH_START_DELAY = 3;
constexpr std::uint64_t MEDIUM_START_DELAY = 2;

bool parseBooleanArgument(
    const std::string &value,
    bool &result)
{
    if (value == "1" ||
        value == "true" ||
        value == "on")
    {
        result = true;
        return true;
    }

    if (value == "0" ||
        value == "false" ||
        value == "off")
    {
        result = false;
        return true;
    }

    return false;
}

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
     * 使用方式：
     *
     * ./priority_inheritance_benchmark \
     *     <pi_enabled> \
     *     <medium_iterations>
     *
     * 範例：
     *
     * ./priority_inheritance_benchmark 1 100
     * ./priority_inheritance_benchmark 0 100
     */
    if (argc != 3)
    {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <pi_enabled> <medium_iterations>\n"
            << "\n"
            << "pi_enabled:\n"
            << "  1 / true / on   Enable priority inheritance\n"
            << "  0 / false / off Disable priority inheritance\n"
            << "\n"
            << "Example:\n"
            << "  "
            << argv[0]
            << " 1 100\n";

        return EXIT_FAILURE;
    }

    bool priority_inheritance_enabled = true;

    if (!parseBooleanArgument(
            argv[1],
            priority_inheritance_enabled))
    {
        std::cerr
            << "Invalid pi_enabled value: "
            << argv[1]
            << '\n';

        return EXIT_FAILURE;
    }

    std::size_t medium_iterations = 0;

    if (!parseSizeArgument(
            argv[2],
            medium_iterations))
    {
        std::cerr
            << "Invalid medium_iterations value: "
            << argv[2]
            << '\n';

        return EXIT_FAILURE;
    }

    /*
     * 開啟或關閉 Priority Inheritance。
     */
    rtos::setPriorityInheritanceEnabled(
        priority_inheritance_enabled);

    rtos::Mutex mutex;

    /*
     * Benchmark 結果。
     */
    std::uint64_t high_request_tick = 0;
    std::uint64_t high_acquire_tick = 0;
    std::uint64_t high_wait_ticks = 0;

    bool high_lock_succeeded = false;

    /*
     * HighTask：
     *
     * 1. 先 delay，讓 LowTask 有機會取得 mutex。
     * 2. 記錄開始要求 mutex 的 tick。
     * 3. 呼叫 lock；若 mutex 已被持有，會阻塞。
     * 4. lock 返回時，記錄取得 mutex 的 tick。
     */
    rtos::createTask(
        "HighTask",
        HIGH_PRIORITY,
        [&]()
        {
            rtos::delay(
                HIGH_START_DELAY);

            high_request_tick =
                rtos::currentTick();

            high_lock_succeeded =
                rtos::lock(
                    mutex);

            high_acquire_tick =
                rtos::currentTick();

            high_wait_ticks =
                high_acquire_tick -
                high_request_tick;

            if (high_lock_succeeded)
            {
                rtos::unlock(
                    mutex);
            }
        });

    /*
     * MediumTask：
     *
     * 在 HighTask blocked 後開始執行。
     *
     * PI disabled：
     * MediumTask priority 2 高於 LowTask priority 1，
     * 因此會延後 LowTask 釋放 mutex。
     *
     * PI enabled：
     * LowTask 繼承 HighTask 的 priority 3，
     * 因此 LowTask 會先於 MediumTask 執行。
     */
    rtos::createTask(
        "MediumTask",
        MEDIUM_PRIORITY,
        [&]()
        {
            rtos::delay(
                MEDIUM_START_DELAY);

            for (std::size_t iteration = 0;
                 iteration < medium_iterations;
                 ++iteration)
            {
                rtos::yield();
            }
        });

    /*
     * LowTask：
     *
     * 在 Tick 2 取得 mutex，
     * 然後在 critical section 中進行固定次數的 yield。
     */
    rtos::createTask(
        "LowTask",
        LOW_PRIORITY,
        [&]()
        {
            const bool lock_succeeded =
                rtos::lock(
                    mutex);

            if (!lock_succeeded)
            {
                return;
            }

            for (std::size_t iteration = 0;
                 iteration <
                     LOW_CRITICAL_ITERATIONS;
                 ++iteration)
            {
                rtos::yield();
            }

            rtos::unlock(
                mutex);
        });

    /*
     * 最大 tick 必須大於：
     *
     * MediumTask workload
     * + LowTask critical section
     * + Task 啟動與結束所需 tick
     */
    const std::uint64_t max_ticks =
        static_cast<std::uint64_t>(
            medium_iterations) +
        static_cast<std::uint64_t>(
            LOW_CRITICAL_ITERATIONS) +
        100;

    rtos::start(
        max_ticks);

    if (!high_lock_succeeded)
    {
        std::cerr
            << "HighTask failed to acquire mutex\n";

        return EXIT_FAILURE;
    }

    /*
     * 只輸出一行 CSV 資料，方便 shell script 收集。
     *
     * 欄位：
     * pi_enabled
     * medium_iterations
     * low_critical_iterations
     * request_tick
     * acquire_tick
     * wait_ticks
     */
    std::cout
        << (priority_inheritance_enabled
                ? 1
                : 0)
        << ','
        << medium_iterations
        << ','
        << LOW_CRITICAL_ITERATIONS
        << ','
        << high_request_tick
        << ','
        << high_acquire_tick
        << ','
        << high_wait_ticks
        << '\n';

    return EXIT_SUCCESS;
}