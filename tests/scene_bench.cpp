/// Benchmarks des critères de la phase 3 :
///   - M3.1 : un tour du monde flecs qui met à jour 100 000 entités (`Transform` + `Velocity`), en
///     moins d'1 ms ;
///   - M3.2 : les matrices monde de 100 000 entités sur 10 niveaux de hiérarchie, en moins de 2 ms.
///
/// Le second mesure aussi, pour mémoire, le stockage `ChildOf` que l'ADR-0015 a écarté : c'est la
/// comparaison qui justifie le choix, et elle doit rester reproductible.
///
/// Exécutable séparé, hors de `ctest`, comme levain_bench : le chiffre dépend de la machine. Il se
/// mesure en Release sur la machine de référence (SPECS §10) et se consigne dans le journal.

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <functional>
#include <print>
#include <string_view>
#include <vector>

#include <flecs.h>

#include "levain/scene/components.hpp"
#include "levain/scene/scene.hpp"
#include "levain/scene/transform.hpp"

namespace
{

using levain::scene::Transform;
using levain::scene::Velocity;
using levain::scene::WorldTransform;

constexpr int EntityCount = 100'000;
constexpr int GridSide = 316; ///< 316 × 316 ≈ 100 000 : les entités en grille, une par unité.
constexpr int Frames = 500;
constexpr float FrameSeconds = 1.0f / 60.0f;

/// Le critère de M3.2 : 10 niveaux, 10 000 entités par niveau.
constexpr int Levels = 10;
constexpr int PerLevel = EntityCount / Levels;

/// Le système du module, par son nom complet : les mesures « seul » l'exécutent hors du pipeline,
/// pour comparer ce qui est comparable (la requête en cascade de `ChildOf` n'est pas un système).
flecs::system systemNamed(flecs::world& world, const char* name)
{
    return flecs::system{world, world.lookup(name)};
}

/// La médiane d'un tour, en millisecondes. Deux tours avant la mesure : flecs y construit ses
/// caches de requêtes.
double medianMilliseconds(const std::function<void()>& turn)
{
    turn();
    turn();
    std::vector<double> milliseconds;
    milliseconds.reserve(Frames);
    for (int frame = 0; frame < Frames; ++frame)
    {
        const auto start = std::chrono::steady_clock::now();
        turn();
        milliseconds.push_back(
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                .count());
    }
    std::ranges::sort(milliseconds);
    return milliseconds[milliseconds.size() / 2];
}

void measureUpdate()
{
    flecs::world world;
    world.import<levain::scene::SceneModule>();
    for (int i = 0; i < EntityCount; ++i)
    {
        const int column = i % GridSide;
        const int row = i / GridSide;
        world.entity()
            .set(Transform{.position = {static_cast<float>(column), 0.0f, static_cast<float>(row)}})
            .set(Velocity{.linear = {0.0f, 1.0f, 0.0f}});
    }

    flecs::system applyVelocity = systemNamed(world, "levain::scene::SceneModule::ApplyVelocity");
    std::print("M3.1 — {} entités (Transform + Velocity), {} tours : ApplyVelocity seul {:.3f} ms, "
               "tour complet {:.3f} ms (les matrices monde de M3.2 comprises)\n",
               EntityCount, Frames, medianMilliseconds([&applyVelocity] { applyVelocity.run(); }),
               medianMilliseconds([&world] { world.progress(FrameSeconds); }));
}

/// Une hiérarchie de `Levels` niveaux de `PerLevel` entités. `childrenPerParent` décide de sa
/// forme : 1 fait 10 000 chaînes indépendantes (le pire cas pour `ChildOf`, un parent par enfant),
/// 10 un arbre large où 1 000 entités d'un niveau se partagent les 10 000 du suivant.
///
/// `useChildOf` bascule sur le stockage écarté par l'ADR-0015, pour la comparaison.
std::vector<flecs::entity> spawnHierarchy(flecs::world& world, int childrenPerParent,
                                          bool useChildOf)
{
    std::vector<flecs::entity> previous;
    std::vector<flecs::entity> current;
    for (int level = 0; level < Levels; ++level)
    {
        current.clear();
        for (int i = 0; i < PerLevel; ++i)
        {
            const flecs::entity parent =
                level == 0 ? flecs::entity{} : previous[i / childrenPerParent];
            flecs::entity entity;
            if (level == 0)
            {
                entity = world.entity();
            }
            else if (useChildOf)
            {
                entity = world.entity().child_of(parent);
            }
            else
            {
                entity = world.entity(flecs::Parent{parent}, nullptr);
            }
            entity.set(Transform{.position = {1.0f, 0.5f, 0.0f}});
            current.push_back(entity);
        }
        previous = current;
    }
    return previous;
}

void measureHierarchy(int childrenPerParent, std::string_view shape)
{
    flecs::world world;
    world.import<levain::scene::SceneModule>();
    spawnHierarchy(world, childrenPerParent, false);

    flecs::system compute =
        systemNamed(world, "levain::scene::SceneModule::ComputeWorldTransforms");
    std::print(
        "M3.2 — {} entités sur {} niveaux, {} : tour complet {:.3f} ms, système seul "
        "{:.3f} ms, {} tables\n",
        EntityCount, Levels, shape, medianMilliseconds([&world] { world.progress(FrameSeconds); }),
        medianMilliseconds([&compute] { compute.run(); }), ecs_get_world_info(world)->table_count);
}

/// Le stockage écarté : une relation `ChildOf` et une requête en cascade. Le système du moteur ne
/// la voit pas (ses entités n'ont pas de `flecs::Parent`), la requête est donc montée ici.
void measureChildOf(int childrenPerParent, std::string_view shape)
{
    flecs::world world;
    world.import<levain::scene::SceneModule>();
    spawnHierarchy(world, childrenPerParent, true);
    const flecs::query<const Transform, const WorldTransform*, WorldTransform> cascade =
        world.query_builder<const Transform, const WorldTransform*, WorldTransform>()
            .term_at(1)
            .parent()
            .cascade()
            .build();

    const double milliseconds = medianMilliseconds(
        [&cascade]
        {
            cascade.each(
                [](const Transform& local, const WorldTransform* parent, WorldTransform& transform)
                {
                    transform.matrix = levain::scene::worldMatrix(
                        parent != nullptr ? parent->matrix : glm::mat4{1.0f}, local);
                });
        });
    std::print("       pour mémoire, ChildOf + cascade, {} : requête seule {:.3f} ms, {} tables\n",
               shape, milliseconds, ecs_get_world_info(world)->table_count);
}

} // namespace

int main()
try
{
    measureUpdate();
    measureHierarchy(1, "chaînes (1 enfant par parent)");
    measureHierarchy(10, "arbre large (10 enfants par parent)");
    measureChildOf(1, "chaînes");
    measureChildOf(10, "arbre large");
    return 0;
}
catch (const std::exception& e)
{
    std::fputs(e.what(), stderr);
    std::fputc('\n', stderr);
    return 1;
}
