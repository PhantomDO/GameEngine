#include <chrono>

#include <doctest/doctest.h>

#include "levain/platform/window.hpp"

using namespace std::chrono_literals;

// SDL_VIDEO_DRIVER=offscreen (tests/CMakeLists.txt) : une vraie fenêtre SDL, sans écran.

TEST_CASE("waitEvents rend la main à l'échéance quand aucun événement n'arrive")
{
    // Le cas d'une fenêtre masquée sur un bureau verrouillé : plus rien n'arrive, et sans limite
    // l'attente ne reviendrait jamais.
    auto window = levain::platform::createWindow("Levain", 64, 64);
    REQUIRE(window.has_value());
    // Ce que la création a mis dans la file : sinon l'attente reviendrait tout de suite.
    (void)levain::platform::pollEvents(*window);

    const auto start = std::chrono::steady_clock::now();
    const levain::platform::Events events = levain::platform::waitEvents(*window, 0.05);
    const auto elapsed = std::chrono::steady_clock::now() - start;

    CHECK(events.window.empty());
    CHECK(elapsed >= 40ms); // a bien dormi, pas une attente nulle
    CHECK(elapsed < 5s);
}
