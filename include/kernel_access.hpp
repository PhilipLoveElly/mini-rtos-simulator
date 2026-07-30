#pragma once

class Scheduler;

namespace rtos
{
namespace detail
{

Scheduler &getKernelScheduler();

}
}