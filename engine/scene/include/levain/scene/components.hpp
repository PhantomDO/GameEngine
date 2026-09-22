#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace levain::scene
{

/// Où est une entité, comment elle est tournée, et à quelle échelle. Dans le repère du monde tant
/// qu'il n'y a pas de hiérarchie (M3.2).
struct Transform
{
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; ///< (w, x, y, z) : l'identité, aucune rotation.
    glm::vec3 scale{1.0f};
};

/// Déplacement par seconde, dans le repère du monde.
struct Velocity
{
    glm::vec3 linear{0.0f};
};

} // namespace levain::scene
