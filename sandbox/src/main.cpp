#include <cstdio>
#include <exception>
#include <print>

#include "levain/core/log.hpp"
#include "levain/core/profile.hpp"
#include "levain/core/version.hpp"
#include "levain/platform/window.hpp"

namespace
{

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

void runMainLoop(levain::platform::Window& window)
{
    LoopState state;

    while (state.isRunning)
    {
        // Masquée, il n'y a rien à dessiner : on dort jusqu'au prochain événement au lieu de
        // tourner à vide.
        const auto events = state.isVisible ? levain::platform::pollEvents(window)
                                            : levain::platform::waitEvents(window);
        for (const auto& event : events)
        {
            applyWindowEvent(state, event);
        }

        // Le rendu viendra ici en M1.2. D'ici là, rien ne cadence la boucle : elle tourne aussi
        // vite que le processeur le permet. C'est le present de la swapchain, calé sur le
        // rafraîchissement de l'écran, qui la ralentira.

        LEVAIN_PROFILE_FRAME();
    }
}

} // namespace

int main()
{
    // std::print peut lever : format_error sur une chaîne de format invalide, system_error si
    // l'écriture échoue. On rattrape au sommet (ADR-0008).
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
