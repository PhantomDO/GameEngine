#include "levain/render/gpu_timer.hpp"

namespace levain::render
{

GpuTimer createGpuTimer(nvrhi::IDevice& device)
{
    GpuTimer timer;
    for (nvrhi::TimerQueryHandle& query : timer.queries)
    {
        query = device.createTimerQuery();
    }
    return timer;
}

std::optional<double> beginGpuTimer(nvrhi::IDevice& device, nvrhi::ICommandList& commandList,
                                    GpuTimer& timer)
{
    nvrhi::ITimerQuery* query = timer.queries[timer.next];

    std::optional<double> previous;
    if (timer.isPending[timer.next])
    {
        if (device.pollTimerQuery(query))
        {
            previous = static_cast<double>(device.getTimerQueryTime(query)) * 1000.0;
        }
        device.resetTimerQuery(query);
    }

    commandList.beginTimerQuery(query);
    timer.isPending[timer.next] = true;
    return previous;
}

void endGpuTimer(nvrhi::ICommandList& commandList, GpuTimer& timer)
{
    commandList.endTimerQuery(timer.queries[timer.next]);
    timer.next = (timer.next + 1) % timer.queries.size();
}

} // namespace levain::render
