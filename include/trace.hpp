#pragma once

#ifdef RTOS_ENABLE_TRACE

#include <iostream>

#define RTOS_TRACE(stream_expression) \
    do                                \
    {                                 \
        std::cout << stream_expression; \
    } while (false)

#else

#define RTOS_TRACE(stream_expression) \
    do                                \
    {                                 \
    } while (false)

#endif