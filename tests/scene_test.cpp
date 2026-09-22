#include <doctest/doctest.h>
#include <flecs.h>

#include "levain/scene/components.hpp"
#include "levain/scene/motion.hpp"
#include "levain/scene/scene.hpp"

using levain::scene::Transform;
using levain::scene::Velocity;

TEST_CASE("applyVelocity avance la position de la vitesse multipliée par la durée")
{
    Transform transform;
    transform.position = {1.0f, 2.0f, 3.0f};
    levain::scene::applyVelocity(transform, Velocity{.linear = {2.0f, 0.0f, -4.0f}}, 0.5f);

    CHECK(transform.position.x == doctest::Approx(2.0f));
    CHECK(transform.position.y == doctest::Approx(2.0f));
    CHECK(transform.position.z == doctest::Approx(1.0f));
}

TEST_CASE("le système ApplyVelocity déplace les entités à chaque tour du monde")
{
    flecs::world world;
    world.import<levain::scene::SceneModule>();
    const flecs::entity moving =
        world.entity().set(Transform{}).set(Velocity{.linear = {4.0f, 0.0f, 0.0f}});
    const flecs::entity still = world.entity().set(Transform{});

    world.progress(0.25f);

    CHECK(moving.get<Transform>().position.x == doctest::Approx(1.0f));
    CHECK(still.get<Transform>().position.x == doctest::Approx(0.0f)); // sans Velocity, rien
}

TEST_CASE("le système ApplyVelocity tourne dans la phase OnUpdate")
{
    flecs::world world;
    world.import<levain::scene::SceneModule>();
    const flecs::entity system = world.lookup("levain::scene::SceneModule::ApplyVelocity");

    REQUIRE(system.is_valid());
    CHECK(system.has(flecs::DependsOn, flecs::OnUpdate));
}
