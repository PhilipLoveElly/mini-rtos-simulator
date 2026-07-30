#include "semaphore.hpp"

#include <stdexcept>

namespace rtos
{

Semaphore::Semaphore(
    std::uint32_t initial_count,
    std::uint32_t max_count)
    : count_(initial_count),
      max_count_(max_count)
{
    if (max_count == 0)
    {
        throw std::invalid_argument(
            "Semaphore max_count must be greater than zero");
    }

    if (initial_count > max_count)
    {
        throw std::invalid_argument(
            "Semaphore initial_count exceeds max_count");
    }
}

}