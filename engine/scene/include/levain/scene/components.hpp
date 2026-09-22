#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace levain::scene
{

/// Où est une entité, comment elle est tournée, et à quelle échelle, **dans le repère de son
/// parent** (ou du monde, si elle n'en a pas). C'est ce qu'on écrit ; la place dans le monde se lit
/// dans `WorldTransform`.
struct Transform
{
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; ///< (w, x, y, z) : l'identité, aucune rotation.
    glm::vec3 scale{1.0f};
};

/// Déplacement par seconde, dans le repère du parent, comme `Transform::position`.
struct Velocity
{
    glm::vec3 linear{0.0f};
};

/// La place de l'entité dans le monde : son `Transform` composé avec ceux de tous ses parents.
/// **Calculée** à chaque tour par le système `ComputeWorldTransforms` (phase `PostUpdate`) : la
/// modifier à la main ne sert à rien, elle sera réécrite. C'est elle que lit le rendu.
struct WorldTransform
{
    glm::mat4 matrix{1.0f};
};

} // namespace levain::scene
