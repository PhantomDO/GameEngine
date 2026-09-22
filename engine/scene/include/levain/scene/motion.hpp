#pragma once

#include "levain/scene/components.hpp"

namespace levain::scene
{

/// Avance `transform` de ce que `velocity` parcourt en `seconds`. Dans l'en-tête : le système la
/// rappelle pour chaque entité, et elle doit pouvoir être inlinée dans sa boucle.
inline void applyVelocity(Transform& transform, const Velocity& velocity, float seconds)
{
    transform.position += velocity.linear * seconds;
}

} // namespace levain::scene
