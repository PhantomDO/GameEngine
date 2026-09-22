#include "levain/scene/scene.hpp"

#include <cstddef>
#include <cstdint>

#include "levain/scene/components.hpp"
#include "levain/scene/motion.hpp"

namespace levain::scene
{

namespace
{

/// La réflexion (addon meta de flecs) : les champs de chaque composant, par leur décalage.
/// L'explorer s'en sert pour afficher les valeurs et les modifier, la sérialisation JSON aussi.
///
/// Décalages explicites plutôt que l'ordre de déclaration : glm::quat range ses floats (x, y, z, w)
/// en mémoire, pas dans l'ordre de son constructeur (w, x, y, z). La surcharge par pointeur de
/// membre de flecs calcule le sien en déréférençant un pointeur nul, ce que UBSan signalerait.
/// Le nombre d'éléments d'un champ, pour flecs : 0 est un scalaire. 1 en ferait un tableau d'un
/// élément, sérialisé « "x":[2.5] » au lieu de « "x":2.5 ».
constexpr std::int32_t ScalarMember = 0;

void describeComponents(flecs::world& world)
{
    world.component<glm::vec3>("vec3")
        .member<float>("x", ScalarMember, offsetof(glm::vec3, x))
        .member<float>("y", ScalarMember, offsetof(glm::vec3, y))
        .member<float>("z", ScalarMember, offsetof(glm::vec3, z));
    world.component<glm::quat>("quat")
        .member<float>("x", ScalarMember, offsetof(glm::quat, x))
        .member<float>("y", ScalarMember, offsetof(glm::quat, y))
        .member<float>("z", ScalarMember, offsetof(glm::quat, z))
        .member<float>("w", ScalarMember, offsetof(glm::quat, w));
    world.component<Transform>()
        .member<glm::vec3>("position", ScalarMember, offsetof(Transform, position))
        .member<glm::quat>("rotation", ScalarMember, offsetof(Transform, rotation))
        .member<glm::vec3>("scale", ScalarMember, offsetof(Transform, scale));
    world.component<Velocity>().member<glm::vec3>("linear", ScalarMember,
                                                  offsetof(Velocity, linear));
}

} // namespace

SceneModule::SceneModule(flecs::world& world)
{
    world.module<SceneModule>();
    describeComponents(world);

    // La glu : une instruction par système, la logique vit dans motion.hpp (ADR-0011). OnUpdate est
    // la phase de la simulation dans le pipeline par défaut de flecs, entre PreUpdate et OnValidate
    // (https://www.flecs.dev/flecs/Systems.html, section « Builtin Pipeline »).
    world.system<Transform, const Velocity>("ApplyVelocity")
        .kind(flecs::OnUpdate)
        .each([](flecs::iter& it, std::size_t, Transform& transform, const Velocity& velocity)
              { applyVelocity(transform, velocity, it.delta_time()); });
}

} // namespace levain::scene
