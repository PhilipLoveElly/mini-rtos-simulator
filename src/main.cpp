#include "message_queue.hpp"

#include <iostream>
#include <stdexcept>

int main()
{
    try
    {
        std::cout
            << "Creating queue with capacity 0\n";

        rtos::MessageQueue<int> queue(0);

        std::cout
            << "ERROR: queue creation unexpectedly succeeded\n";

        return 1;
    }
    catch (const std::invalid_argument &error)
    {
        std::cout
            << "Caught expected exception: "
            << error.what()
            << '\n';
    }

    std::cout
        << "Capacity-zero test passed\n";

    return 0;
}