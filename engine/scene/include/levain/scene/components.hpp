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

/// L'état de l'entité au pas de simulation précédent, pour que le rendu affiche l'entre-deux au
/// lieu de sauter d'un pas à l'autre. Attaché **automatiquement** à ce que la simulation déplace,
/// c'est-à-dire à toute entité qui a une `Velocity` (ADR-0016) ; une entité sans lui est rendue
/// telle quelle, sans surcoût.
///
/// **Pourquoi une struct qui n'enveloppe qu'un `Transform`** : flecs reconnaît un composant par son
/// type C++, donc deux `Transform` sur la même entité demandent deux types. flecs sait aussi le
/// faire sans type supplémentaire, par une paire `(Transform, Previous)` — plus idiomatique, et la
/// réflexion y est partagée. On garde l'enveloppe parce qu'elle est **explicite à la relecture** :
/// dans un système qui interpole, les deux paramètres d'une paire sont deux `const Transform&` que
/// rien n'empêche d'intervertir, et le bug ne se voit qu'à l'œil, une image sur deux. Choix de
/// Donnovan le 22/09/2026 ; le tableau des compromis est dans `docs/QA.md`.
struct PreviousTransform
{
    Transform transform;
};

/// Où en est le rendu entre le pas de simulation précédent (0) et l'actuel (1). Singleton du monde,
/// posé à chaque image par `advanceWorld` : c'est le reste de l'accumulateur.
struct RenderAlpha
{
    float value = 1.0f;
};

/// La place de l'entité dans le monde : son `Transform` composé avec ceux de tous ses parents.
///
/// Ce n'est pas le même contenu sous une autre forme : une matrice peut porter un **cisaillement**
/// — qu'une hiérarchie fabrique dès qu'une échelle non uniforme rencontre une rotation — et un
/// `Transform` ne sait pas le représenter. Le `Transform` est la source, la matrice le produit
/// (`docs/QA.md`).
/// **Calculée** à chaque tour par le système `ComputeWorldTransforms` (phase `PostUpdate`) : la
/// modifier à la main ne sert à rien, elle sera réécrite. C'est elle que lit le rendu.
struct WorldTransform
{
    glm::mat4 matrix{1.0f};
};

} // namespace levain::scene
