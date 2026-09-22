#include <string_view>

#include <doctest/doctest.h>
#include <flecs.h>
#include <glm/gtc/constants.hpp>

#include "levain/scene/components.hpp"
#include "levain/scene/motion.hpp"
#include "levain/scene/scene.hpp"
#include "levain/scene/transform.hpp"

using levain::scene::Transform;
using levain::scene::Velocity;
using levain::scene::worldPosition;
using levain::scene::WorldTransform;

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

TEST_CASE("les champs des composants se lisent et s'écrivent en JSON, comme dans l'explorer")
{
    flecs::world world;
    world.import<levain::scene::SceneModule>();
    Transform transform;
    transform.position = {2.5f, 0.0f, 0.0f};
    const flecs::entity entity = world.entity("cube").set(transform);

    // L'explorer lit le monde en JSON (addon REST) : sans la réflexion, il ne verrait qu'un bloc
    // d'octets.
    const flecs::string json = entity.to_json();
    CHECK(std::string_view{json.c_str()}.find(R"("position":{"x":2.5)") != std::string_view::npos);

    // Et il écrit de la même façon : c'est ce que fait l'explorer quand on modifie un champ.
    entity.set_json<Transform>(R"({"position":{"x":7, "y":1, "z":0}})");
    CHECK(entity.get<Transform>().position.x == doctest::Approx(7.0f));
    CHECK(entity.get<Transform>().position.y == doctest::Approx(1.0f));
    CHECK(entity.get<Transform>().rotation.w ==
          doctest::Approx(1.0f)); // champs absents : inchangés
}

TEST_CASE("localMatrix compose l'échelle, la rotation puis la translation")
{
    Transform transform;
    transform.position = {10.0f, 0.0f, 0.0f};
    transform.rotation = glm::angleAxis(glm::half_pi<float>(), glm::vec3{0.0f, 1.0f, 0.0f});
    transform.scale = {2.0f, 2.0f, 2.0f};

    // Un quart de tour autour de Y met l'axe X (mis à l'échelle) sur -Z, et la translation reste
    // celle du parent : c'est l'ordre échelle, rotation, translation.
    const glm::vec4 point =
        levain::scene::localMatrix(transform) * glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};
    CHECK(point.x == doctest::Approx(10.0f));
    CHECK(point.z == doctest::Approx(-2.0f));
}

TEST_CASE("un enfant suit son parent déplacé")
{
    flecs::world world;
    world.import<levain::scene::SceneModule>();
    const flecs::entity parent =
        world.entity("parent").set(Transform{.position = {1.0f, 0.0f, 0.0f}});
    // La hiérarchie passe par le composant flecs::Parent, jamais par child_of (ADR-0015).
    const flecs::entity child = world.entity(flecs::Parent{parent}, "enfant")
                                    .set(Transform{.position = {0.0f, 2.0f, 0.0f}});

    world.progress(0.0f);
    CHECK(worldPosition(child.get<WorldTransform>()).x == doctest::Approx(1.0f));
    CHECK(worldPosition(child.get<WorldTransform>()).y == doctest::Approx(2.0f));

    parent.set(Transform{.position = {5.0f, 0.0f, 0.0f}});
    world.progress(0.0f);
    CHECK(worldPosition(child.get<WorldTransform>()).x == doctest::Approx(5.0f));
    CHECK(worldPosition(child.get<WorldTransform>()).y == doctest::Approx(2.0f));

    // Le Transform de l'enfant, lui, n'a pas bougé : il est dans le repère de son parent.
    CHECK(child.get<Transform>().position.x == doctest::Approx(0.0f));
}

TEST_CASE("un enfant tourne et grandit avec son parent")
{
    flecs::world world;
    world.import<levain::scene::SceneModule>();
    const flecs::entity parent = world.entity().set(
        Transform{.rotation = glm::angleAxis(glm::half_pi<float>(), glm::vec3{0.0f, 1.0f, 0.0f}),
                  .scale = {3.0f, 3.0f, 3.0f}});
    const flecs::entity child =
        world.entity(flecs::Parent{parent}, nullptr).set(Transform{.position = {1.0f, 0.0f, 0.0f}});

    world.progress(0.0f);

    // Un mètre devant un parent tourné d'un quart de tour et trois fois plus grand : trois mètres
    // sur -Z.
    CHECK(worldPosition(child.get<WorldTransform>()).x == doctest::Approx(0.0f));
    CHECK(worldPosition(child.get<WorldTransform>()).z == doctest::Approx(-3.0f));
}

TEST_CASE("un petit-enfant suit, même si la hiérarchie est bâtie dans le désordre")
{
    flecs::world world;
    world.import<levain::scene::SceneModule>();
    const Transform oneOnX{.position = {1.0f, 0.0f, 0.0f}};
    // L'ordre de création met la table du petit-enfant dans le cache de la requête avant celle de
    // son parent : sans le tri par profondeur (EcsQueryGroupByOrdered), le petit-enfant est calculé
    // avec la matrice de la frame précédente et se retrouve à x = 1 au lieu de 3.
    const flecs::entity child = world.entity().set(oneOnX);
    const flecs::entity grandChild = world.entity(flecs::Parent{child}, nullptr).set(oneOnX);
    const flecs::entity root = world.entity().set(oneOnX);
    child.set(flecs::Parent{root});

    world.progress(0.0f);

    CHECK(worldPosition(grandChild.get<WorldTransform>()).x == doctest::Approx(3.0f));
}

TEST_CASE("le WorldTransform vient avec le Transform, et se lit en JSON")
{
    flecs::world world;
    world.import<levain::scene::SceneModule>();
    const flecs::entity entity = world.entity().set(Transform{.position = {2.0f, 0.0f, 0.0f}});

    // Le trait With : personne n'ajoute WorldTransform à la main.
    REQUIRE(entity.has<WorldTransform>());
    world.progress(0.0f);

    // Et l'explorer le voit, comme les autres composants (réflexion de la matrice).
    const flecs::string json = entity.to_json();
    CHECK(std::string_view{json.c_str()}.find(R"("matrix":[)") != std::string_view::npos);
}
