/// Benchmark des allocateurs contre `malloc`.
///
/// Exécutable séparé, volontairement hors de `ctest` : un chiffre de performance dépend de
/// la machine et de sa charge, il n'a rien à faire dans un test qui doit être binaire.
/// Les chiffres du journal viennent d'ici, sur la machine de référence (SPECS §10).

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <print>
#include <vector>

#include "levain/core/linear_allocator.hpp"
#include "levain/core/pool_allocator.hpp"

namespace
{

constexpr std::size_t AllocationsPerRun = 100'000;
constexpr std::size_t BlockSize = 64;
constexpr int Runs = 5;

using Nanoseconds = std::chrono::nanoseconds;

/// Médiane de `Runs` exécutions : moins sensible qu'une moyenne à un pic d'ordonnancement.
Nanoseconds median(std::vector<Nanoseconds> samples)
{
    std::ranges::sort(samples);

    return samples[samples.size() / 2];
}

template <typename Body> Nanoseconds timeBest(Body&& body)
{
    std::vector<Nanoseconds> samples;
    samples.reserve(Runs);

    for (int run = 0; run < Runs; ++run)
    {
        const auto start = std::chrono::steady_clock::now();
        body();
        samples.push_back(
            std::chrono::duration_cast<Nanoseconds>(std::chrono::steady_clock::now() - start));
    }

    return median(std::move(samples));
}

void report(const char* name, Nanoseconds total)
{
    const double perAllocation =
        static_cast<double>(total.count()) / static_cast<double>(AllocationsPerRun);

    std::print("{:<28} {:>9.2f} ns/alloc  {:>8} µs au total\n", name, perAllocation,
               std::chrono::duration_cast<std::chrono::microseconds>(total).count());
}

} // namespace

int main()
try
{
    std::print("{} allocations de {} octets, médiane de {} exécutions\n\n", AllocationsPerRun,
               BlockSize, Runs);

    // `malloc` + `free` appariés : le cas de référence, celui qu'un moteur écrit sans y
    // penser et qui coûte le plus cher dans une boucle de jeu.
    const auto mallocTime = timeBest(
        []
        {
            std::vector<void*> blocks;
            blocks.reserve(AllocationsPerRun);

            for (std::size_t i = 0; i < AllocationsPerRun; ++i)
            {
                blocks.push_back(std::malloc(BlockSize));
            }
            for (void* block : blocks)
            {
                std::free(block);
            }
        });

    levain::core::LinearAllocator arena{AllocationsPerRun * BlockSize * 2};
    const auto linearTime = timeBest(
        [&arena]
        {
            arena.reset();
            for (std::size_t i = 0; i < AllocationsPerRun; ++i)
            {
                [[maybe_unused]] volatile auto* block = arena.allocate(BlockSize, 16);
            }
        });

    levain::core::PoolAllocator pool{BlockSize, 16, AllocationsPerRun};
    const auto poolTime = timeBest(
        [&pool]
        {
            std::vector<void*> blocks;
            blocks.reserve(AllocationsPerRun);

            for (std::size_t i = 0; i < AllocationsPerRun; ++i)
            {
                blocks.push_back(pool.allocate());
            }
            for (void* block : blocks)
            {
                pool.deallocate(block);
            }
        });

    report("malloc + free", mallocTime);
    report("LinearAllocator", linearTime);
    report("PoolAllocator (alloc + free)", poolTime);

    std::print("\nLinearAllocator : {:.1f}x plus rapide que malloc\n",
               static_cast<double>(mallocTime.count()) / static_cast<double>(linearTime.count()));
    std::print("PoolAllocator   : {:.1f}x plus rapide que malloc\n",
               static_cast<double>(mallocTime.count()) / static_cast<double>(poolTime.count()));

    return 0;
}
// std::print peut lever ; une exception qui s'échappe de main appelle std::terminate.
// Function-try-block plutôt qu'un try imbriqué : le corps reste à un seul niveau
// d'indentation (ADR-0008).
catch (const std::exception& e)
{
    std::fputs(e.what(), stderr);
    std::fputc('\n', stderr);

    return 1;
}
