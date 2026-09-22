#pragma once

#include <flecs.h>

namespace levain::scene
{

/// Le module flecs de la scène : ses composants et ses systèmes, rangés par phase du pipeline de
/// flecs. S'installe par `world.import<levain::scene::SceneModule>()`.
///
/// Les modules de flecs : manuel, section « Modules »
/// (https://www.flecs.dev/flecs/md_docs_2Manual.html).
struct SceneModule
{
    explicit SceneModule(flecs::world& world);
};

} // namespace levain::scene
