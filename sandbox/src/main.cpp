#include <chrono>
#include <cstdio>
#include <exception>
#include <format>
#include <print>
#include <string>

#include "levain/core/frame_time.hpp"
#include "levain/core/log.hpp"
#include "levain/core/profile.hpp"
#include "levain/core/version.hpp"
#include "levain/platform/window.hpp"

namespace
{

using Clock = std::chrono::steady_clock;

/// Durée sur laquelle le frame time du titre est résumé.
constexpr double FrameTimePeriodSeconds = 1.0;

struct LoopState
{
    bool isRunning = true;
    bool isVisible = true;
};

void applyWindowEvent(LoopState& state, const levain::platform::WindowEvent& event)
{
    using levain::platform::WindowEventType;

    switch (event.type)
    {
    case WindowEventType::CloseRequested:
        state.isRunning = false;
        break;
    // Les deux arrivent en double : sous Wayland, SDL renvoie EXPOSED après chaque
    // redimensionnement. On ne journalise donc que les changements d'état.
    case WindowEventType::Hidden:
        if (state.isVisible)
        {
            levain::core::log("sandbox", levain::core::LogLevel::Info, "masquée : boucle en pause");
        }
        state.isVisible = false;
        break;
    case WindowEventType::Shown:
        if (!state.isVisible)
        {
            levain::core::log("sandbox", levain::core::LogLevel::Info, "visible : boucle relancée");
        }
        state.isVisible = true;
        break;
    case WindowEventType::Resized:
        // M1.2 : c'est ici que la swapchain sera recréée à la nouvelle taille.
        levain::core::log("sandbox", levain::core::LogLevel::Info, "redimensionnée : {} × {} px",
                          event.pixelSize.width, event.pixelSize.height);
        break;
    }
}

double secondsBetween(Clock::time_point start, Clock::time_point end)
{
    return std::chrono::duration<double>(end - start).count();
}

std::string describeFrameTimes(const levain::core::FrameTimeSummary& summary)
{
    // Tirets ASCII : setWindowTitle refuse le reste (voir window.hpp). averageMs n'est jamais
    // nul, un résumé couvre au moins FrameTimePeriodSeconds.
    return std::format("Levain - {:.3f} ms (min {:.3f}, max {:.3f}) - {:.0f} images/s",
                       summary.averageMs, summary.minMs, summary.maxMs, 1000.0 / summary.averageMs);
}

void runMainLoop(levain::platform::Window& window)
{
    LoopState state;
    levain::core::FrameTimeAccumulator frameTimes;
    Clock::time_point previousFrameEnd = Clock::now();

    while (state.isRunning)
    {
        if (!state.isVisible)
        {
            for (const auto& event : levain::platform::waitEvents(window))
            {
                applyWindowEvent(state, event);
            }

            // Le temps passé masquée n'est pas une frame. Sans cette remise à l'heure, la
            // première frame après la restauration durerait toute la minimisation, et le
            // maximum affiché serait de plusieurs secondes.
            previousFrameEnd = Clock::now();
            continue;
        }

        {
            LEVAIN_PROFILE_SCOPE_NAMED("événements");

            for (const auto& event : levain::platform::pollEvents(window))
            {
                applyWindowEvent(state, event);
            }
        }

        // Le rendu viendra ici en M1.2. D'ici là, rien ne cadence la boucle : elle tourne aussi
        // vite que le processeur le permet. C'est le present de la swapchain, calé sur le
        // rafraîchissement de l'écran, qui la ralentira.

        const Clock::time_point frameEnd = Clock::now();
        const double frameSeconds = secondsBetween(previousFrameEnd, frameEnd);
        previousFrameEnd = frameEnd;

        if (const auto summary =
                levain::core::recordFrame(frameTimes, frameSeconds, FrameTimePeriodSeconds))
        {
            LEVAIN_PROFILE_SCOPE_NAMED("titre");
            levain::platform::setWindowTitle(window, describeFrameTimes(*summary));
        }

        LEVAIN_PROFILE_FRAME();
    }
}

} // namespace

int main()
{
    // std::print et std::format peuvent lever : format_error sur une chaîne de format
    // invalide, system_error si l'écriture échoue. On rattrape au sommet (ADR-0008).
    try
    {
        std::print("Levain {} — {} — __cplusplus {}\n", levain::core::version(),
                   levain::core::toolchain(), __cplusplus);

        auto window = levain::platform::createWindow("Levain", 1280, 720);
        if (!window)
        {
            levain::core::log("sandbox", levain::core::LogLevel::Critical, "{}",
                              window.error().message);
            return 1;
        }

        runMainLoop(*window);
        levain::core::log("sandbox", levain::core::LogLevel::Info, "fenêtre fermée");
    }
    catch (const std::exception& e)
    {
        std::fputs(e.what(), stderr);
        std::fputc('\n', stderr);
        return 1;
    }

    return 0;
}
