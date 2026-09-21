#include "levain/platform/window.hpp"

#include <format>
#include <optional>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "levain/core/assert.hpp"
#include "levain/core/log.hpp"

namespace levain::platform
{

namespace
{

bool isWindowEvent(const SDL_Event& event)
{
    return event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST;
}

/// Traduit un événement SDL vers les nôtres. Ce qui ne concerne pas cette fenêtre, ou qu'on ne
/// traite pas encore (clavier, souris), est ignoré.
std::optional<WindowEvent> translateEvent(const SDL_Event& event, SDL_WindowID windowId)
{
    // Envoyé par SDL sur SIGINT (Ctrl+C), SIGTERM, et à la fermeture de la dernière fenêtre.
    if (event.type == SDL_EVENT_QUIT)
    {
        return WindowEvent{.type = WindowEventType::CloseRequested};
    }

    // Le filtre sur l'identifiant servira quand l'éditeur ouvrira des fenêtres secondaires
    // (viewports d'ImGui, M7.1) : fermer l'une d'elles ne doit pas quitter le moteur.
    if (!isWindowEvent(event) || event.window.windowID != windowId)
    {
        return std::nullopt;
    }

    switch (event.type)
    {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return WindowEvent{.type = WindowEventType::CloseRequested};

    // PIXEL_SIZE_CHANGED et non RESIZED : RESIZED donne la taille en points, la swapchain a
    // besoin des pixels.
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        return WindowEvent{
            .type = WindowEventType::Resized,
            .pixelSize = {.width = event.window.data1, .height = event.window.data2}};

    // Wayland ne dit jamais à une application qu'elle a été minimisée : le compositeur la
    // « suspend », ce que SDL traduit en OCCLUDED, puis la réveille avec EXPOSED. X11 envoie
    // MINIMIZED et RESTORED. On accepte les deux paires : le moteur n'a besoin que de savoir
    // s'il y a quelque chose à dessiner.
    case SDL_EVENT_WINDOW_MINIMIZED:
    case SDL_EVENT_WINDOW_OCCLUDED:
        return WindowEvent{.type = WindowEventType::Hidden};

    case SDL_EVENT_WINDOW_RESTORED:
    case SDL_EVENT_WINDOW_EXPOSED:
        return WindowEvent{.type = WindowEventType::Shown};

    default:
        return std::nullopt;
    }
}

void appendPendingEvents(std::vector<WindowEvent>& events, SDL_WindowID windowId)
{
    SDL_Event event{};
    while (SDL_PollEvent(&event))
    {
        if (const auto translated = translateEvent(event, windowId))
        {
            events.push_back(*translated);
        }
    }
}

} // namespace

void WindowDeleter::operator()(SDL_Window* window) const noexcept
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

core::Result<Window> createWindow(const std::string& title, int width, int height)
{
    // Chaque fenêtre arrête SDL en se détruisant. Une deuxième fenêtre vivante serait donc
    // privée de SDL à la destruction de la première.
    LEVAIN_ASSERT(SDL_WasInit(SDL_INIT_VIDEO) == 0, "une seule fenêtre à la fois");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        return core::makeError(core::ErrorCode::Unsupported,
                               std::format("SDL_Init : {}", SDL_GetError()));
    }

    // HIGH_PIXEL_DENSITY : sans ce drapeau, sur un écran à 200 %, SDL demande une surface en
    // basse résolution que le compositeur agrandit, et l'image est floue.
    SDL_Window* handle = SDL_CreateWindow(title.c_str(), width, height,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (handle == nullptr)
    {
        std::string message = std::format("SDL_CreateWindow : {}", SDL_GetError());
        SDL_Quit();
        return core::makeError(core::ErrorCode::Unsupported, std::move(message));
    }

    return Window{.handle = std::unique_ptr<SDL_Window, WindowDeleter>{handle}};
}

std::vector<WindowEvent> pollEvents(const Window& window)
{
    std::vector<WindowEvent> events;
    appendPendingEvents(events, SDL_GetWindowID(window.handle.get()));
    return events;
}

std::vector<WindowEvent> waitEvents(const Window& window)
{
    const SDL_WindowID windowId = SDL_GetWindowID(window.handle.get());
    std::vector<WindowEvent> events;

    SDL_Event event{};
    if (!SDL_WaitEvent(&event))
    {
        core::log("platform", core::LogLevel::Warning, "SDL_WaitEvent : {}", SDL_GetError());
    }
    else if (const auto translated = translateEvent(event, windowId))
    {
        events.push_back(*translated);
    }

    appendPendingEvents(events, windowId);
    return events;
}

} // namespace levain::platform
