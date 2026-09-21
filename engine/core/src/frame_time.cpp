#include "levain/core/frame_time.hpp"

#include <algorithm>

namespace levain::core
{

namespace
{

constexpr double toMilliseconds(double seconds)
{
    return seconds * 1000.0;
}

} // namespace

std::optional<FrameTimeSummary> recordFrame(FrameTimeAccumulator& accumulator, double frameSeconds,
                                            double periodSeconds)
{
    accumulator.elapsedSeconds += frameSeconds;
    accumulator.minSeconds = std::min(accumulator.minSeconds, frameSeconds);
    accumulator.maxSeconds = std::max(accumulator.maxSeconds, frameSeconds);
    ++accumulator.frameCount;

    if (accumulator.elapsedSeconds < periodSeconds)
    {
        return std::nullopt;
    }

    const FrameTimeSummary summary{
        .averageMs = toMilliseconds(accumulator.elapsedSeconds / accumulator.frameCount),
        .minMs = toMilliseconds(accumulator.minSeconds),
        .maxMs = toMilliseconds(accumulator.maxSeconds),
        .frameCount = accumulator.frameCount,
    };
    accumulator = {};
    return summary;
}

} // namespace levain::core
