/// Benchmark du critère de M3.1 : un tour du monde flecs qui met à jour 100 000 entités
/// (`Transform` + `Velocity`), en moins d'1 ms.
///
/// Exécutable séparé, hors de `ctest`, comme levain_bench : le chiffre dépend de la machine. Il se
/// mesure en Release sur la machine de référence (SPECS §10) et se consigne dans le journal.

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <print>
#include <vector>

#include <flecs.h>

#include "levain/scene/components.hpp"
#include "levain/scene/scene.hpp"

namespace
{

constexpr int EntityCount = 100'000;
constexpr int GridSide = 316; ///< 316 × 316 ≈ 100 000 : les entités en grille, une par unité.
constexpr int Frames = 500;
constexpr float FrameSeconds = 1.0f / 60.0f;

} // namespace

int main()
try
{
    flecs::world world;
    world.import<levain::scene::SceneModule>();
    for (int i = 0; i < EntityCount; ++i)
    {
        const int column = i % GridSide;
        const int row = i / GridSide;
        world.entity()
            .set(levain::scene::Transform{
                .position = {static_cast<float>(column), 0.0f, static_cast<float>(row)}})
            .set(levain::scene::Velocity{.linear = {0.0f, 1.0f, 0.0f}});
    }

    // Un premier tour hors mesure : flecs y construit ses caches de requêtes.
    world.progress(FrameSeconds);

    std::vector<double> milliseconds;
    milliseconds.reserve(Frames);
    for (int frame = 0; frame < Frames; ++frame)
    {
        const auto start = std::chrono::steady_clock::now();
        world.progress(FrameSeconds);
        milliseconds.push_back(
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                .count());
    }
    std::ranges::sort(milliseconds);

    std::print("{} entités (Transform + Velocity), {} tours du monde : médiane {:.3f} ms, "
               "min {:.3f} ms, max {:.3f} ms\n",
               EntityCount, Frames, milliseconds[milliseconds.size() / 2], milliseconds.front(),
               milliseconds.back());
    return 0;
}
catch (const std::exception& e)
{
    std::fputs(e.what(), stderr);
    std::fputc('\n', stderr);
    return 1;
}
