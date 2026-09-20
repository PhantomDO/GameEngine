#include <chrono>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <print>
#include <thread>

#include "levain/core/linear_allocator.hpp"
#include "levain/core/log.hpp"
#include "levain/core/profile.hpp"
#include "levain/core/version.hpp"

namespace
{

/// Nombre de frames simulées. Le sandbox n'a pas encore de vraie boucle — elle arrive avec
/// la fenêtre en M1.1 — mais il en faut une pour que Tracy ait quelque chose à découper.
constexpr int SimulatedFrames = 120;

/// Imite le travail d'une frame : une arène remise à zéro, des allocations, un peu de
/// calcul. C'est le patron que suivra la vraie boucle.
void simulateFrame(levain::core::LinearAllocator& frameArena)
{
    LEVAIN_PROFILE_SCOPE();

    frameArena.reset();

    {
        LEVAIN_PROFILE_SCOPE_NAMED("allocations de frame");

        for (int i = 0; i < 1000; ++i)
        {
            [[maybe_unused]] volatile auto* block = frameArena.allocate(64, 16);
        }
    }

    {
        LEVAIN_PROFILE_SCOPE_NAMED("travail simulé");
        std::this_thread::sleep_for(std::chrono::microseconds{200});
    }
}

} // namespace

int main()
{
    // std::print peut lever : format_error sur une chaîne de format invalide, system_error
    // si l'écriture échoue. Une exception qui s'échappe de main appelle std::terminate,
    // donc on la rattrape ici (ADR-0008 : les exceptions ne servent pas de contrôle de flux
    // et sont rattrapées au sommet).
    try
    {
        std::print("Levain {} — {} — __cplusplus {}\n", levain::core::version(),
                   levain::core::toolchain(), __cplusplus);

        levain::core::log("sandbox", levain::core::LogLevel::Info,
                          "{} frames simulées, arène de {} Kio", SimulatedFrames, 128);

        levain::core::LinearAllocator frameArena{std::size_t{128} * 1024};

        for (int frame = 0; frame < SimulatedFrames; ++frame)
        {
            simulateFrame(frameArena);
            LEVAIN_PROFILE_FRAME();
        }

        levain::core::log("sandbox", levain::core::LogLevel::Info, "terminé");
    }
    catch (const std::exception& e)
    {
        std::fputs(e.what(), stderr);
        std::fputc('\n', stderr);
        return 1;
    }

    return 0;
}
