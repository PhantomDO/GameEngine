#include <ostream>

#include <doctest/doctest.h>

#include "levain/core/frame_time.hpp"

using levain::core::FrameTimeAccumulator;
using levain::core::FrameTimeSummary;
using levain::core::recordFrame;

TEST_CASE("recordFrame ne rend rien tant que la période n'est pas couverte")
{
    FrameTimeAccumulator accumulator;

    CHECK_FALSE(recordFrame(accumulator, 0.4, 1.0).has_value());
    CHECK_FALSE(recordFrame(accumulator, 0.4, 1.0).has_value());
    CHECK(recordFrame(accumulator, 0.4, 1.0).has_value());
}

TEST_CASE("recordFrame résume moyenne, minimum et maximum en millisecondes")
{
    FrameTimeAccumulator accumulator;

    // Une saccade de 50 ms parmi des frames de 1 ms : c'est le maximum qui la montre.
    for (int i = 0; i < 9; ++i)
    {
        (void)recordFrame(accumulator, 0.001, 0.059);
    }
    // Sans résumé, value_or rend des zéros et les CHECK échouent.
    const auto summary = recordFrame(accumulator, 0.050, 0.059).value_or(FrameTimeSummary{});

    CHECK(summary.frameCount == 10);
    CHECK(summary.averageMs == doctest::Approx(5.9));
    CHECK(summary.minMs == doctest::Approx(1.0));
    CHECK(summary.maxMs == doctest::Approx(50.0));
}

TEST_CASE("recordFrame repart de zéro après chaque résumé")
{
    FrameTimeAccumulator accumulator;
    (void)recordFrame(accumulator, 0.5, 0.5); // période précédente, minimum de 500 ms

    const auto summary = recordFrame(accumulator, 0.8, 0.5).value_or(FrameTimeSummary{});

    CHECK(summary.frameCount == 1);
    CHECK(summary.minMs == doctest::Approx(800.0));
}
