#include "levain/scene/scene.hpp"

#include <cstddef>

#include "levain/scene/components.hpp"
#include "levain/scene/motion.hpp"

namespace levain::scene
{

SceneModule::SceneModule(flecs::world& world)
{
    world.module<SceneModule>();
    world.component<Transform>();
    world.component<Velocity>();

    // La glu : une instruction par système, la logique vit dans motion.hpp (ADR-0011). OnUpdate est
    // la phase de la simulation dans le pipeline par défaut de flecs, entre PreUpdate et OnValidate
    // (https://www.flecs.dev/flecs/Systems.html, section « Builtin Pipeline »).
    world.system<Transform, const Velocity>("ApplyVelocity")
        .kind(flecs::OnUpdate)
        .each([](flecs::iter& it, std::size_t, Transform& transform, const Velocity& velocity)
              { applyVelocity(transform, velocity, it.delta_time()); });
}

} // namespace levain::scene
