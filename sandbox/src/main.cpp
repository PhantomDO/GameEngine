#include <chrono>
#include <cstdio>
#include <exception>
#include <format>
#include <print>
#include <string>

#include "levain/core/assert.hpp"
#include "levain/core/frame_time.hpp"
#include "levain/core/log.hpp"
#include "levain/core/profile.hpp"
#include "levain/core/version.hpp"
#include "levain/gpu/device.hpp"
#include "levain/platform/window.hpp"

namespace
{

using Clock = std::chrono::steady_clock;

/// Durée sur laquelle le frame time du titre est résumé.
constexpr double FrameTimePeriodSeconds = 1.0;

/// Validation en Debug seulement (règle n°4) : elle coûte cher, et c'est là qu'on développe.
constexpr bool EnableValidation = LEVAIN_ASSERTIONS_ENABLED != 0;

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

/// Efface l'image de la swapchain et la présente. Le rendu viendra dans engine/render ; en
/// attendant, c'est tout ce que dessine une frame.
void renderFrame(levain::gpu::GpuDevice& gpu, const levain::platform::Window& window,
                 nvrhi::ICommandList& commandList)
{
    nvrhi::ITexture* backBuffer = levain::gpu::beginFrame(gpu, window);
    if (backBuffer == nullptr)
    {
        return;
    }

    // La couleur de fond, en attendant le premier triangle (M1.3) : une croûte de levain. Locale et
    // non globale : le constructeur de nvrhi::Color n'est pas noexcept, et une exception levée à
    // l'initialisation d'une globale ne se rattrape pas.
    const nvrhi::Color clearColor{0.55f, 0.32f, 0.14f, 1.0f};

    commandList.open();
    commandList.clearTextureFloat(backBuffer, nvrhi::AllSubresources, clearColor);
    commandList.close();
    gpu.nvrhi->executeCommandList(&commandList);

    levain::gpu::presentFrame(gpu);
}

void runMainLoop(levain::platform::Window& window, levain::gpu::GpuDevice& gpu)
{
    const nvrhi::CommandListHandle commandList = gpu.nvrhi->createCommandList();
    LoopState state;
    levain::core::FrameTimeAccumulator frameTimes;
    const Clock::time_point loopStart = Clock::now();
    Clock::time_point previousFrameEnd = loopStart;
    int frameCount = 0;

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

        {
            LEVAIN_PROFILE_SCOPE_NAMED("rendu");
            renderFrame(gpu, window, *commandList);
        }

        const Clock::time_point frameEnd = Clock::now();
        const double frameSeconds = secondsBetween(previousFrameEnd, frameEnd);
        previousFrameEnd = frameEnd;

        if (const auto summary =
                levain::core::recordFrame(frameTimes, frameSeconds, FrameTimePeriodSeconds))
        {
            LEVAIN_PROFILE_SCOPE_NAMED("titre");
            levain::platform::setWindowTitle(window, describeFrameTimes(*summary));
        }

        ++frameCount;
        LEVAIN_PROFILE_FRAME();
    }

    // Lu par la CI, qui échoue si la boucle a tourné moins d'une seconde : un démarrage lent
    // (lavapipe, validation, sanitizers) peut sinon manger tout le délai sans que rien ne rougisse.
    levain::core::log("sandbox", levain::core::LogLevel::Info,
                      "boucle arrêtée après {:.1f} s et {} frames",
                      secondsBetween(loopStart, Clock::now()), frameCount);
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

        // Déclaré après window, gpu sera détruit avant elle : la surface Vulkan doit disparaître
        // avant la fenêtre SDL qui la porte.
        const Clock::time_point deviceStart = Clock::now();
        auto gpu = levain::gpu::createGpuDevice(*window, {.enableValidation = EnableValidation});
        if (!gpu)
        {
            levain::core::log("sandbox", levain::core::LogLevel::Critical, "{}",
                              gpu.error().message);
            return 1;
        }
        levain::core::log("sandbox", levain::core::LogLevel::Info, "device créé en {:.1f} ms",
                          secondsBetween(deviceStart, Clock::now()) * 1000.0);

        runMainLoop(*window, *gpu);
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
