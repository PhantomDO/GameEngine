#pragma once

#include <array>
#include <cstddef>

#include "levain/animation/animation_set.hpp"
#include "levain/animation/pose.hpp"

namespace levain::animation
{

/// La locomotion au sol (#118) : trois clips, repos, marche et course, mélangés selon la vitesse du
/// personnage. `walkSpeed` et `runSpeed` sont les vitesses auxquelles la marche et la course ont
/// été animées : à ces vitesses, un seul clip joue, et les pieds ne glissent pas.
struct Locomotion
{
    std::size_t idle = 0;
    std::size_t walk = 0;
    std::size_t run = 0;
    float walkSpeed = 1.0f;
    float runSpeed = 3.0f;
};

/// Les poids du repos, de la marche et de la course pour `speed`, dans cet ordre : un mélange
/// linéaire entre les deux clips qui encadrent la vitesse. Au-delà de la course, la course seule ;
/// à l'arrêt ou en recul, le repos seul.
[[nodiscard]] std::array<float, 3> strideWeightsOf(const Locomotion& locomotion, float speed);

/// Où en sont les clips. La marche et la course partagent une **phase** : à mi-chemin entre les
/// deux, leurs pas tombent ensemble, au lieu de se contrarier. Le repos garde son propre rythme.
struct LocomotionClock
{
    float stridePhase = 0.0f; ///< Entre 0 et 1, commune à la marche et à la course.
    float idleSeconds = 0.0f;
};

/// Avance les horloges de `seconds` à la vitesse `speed`, et rend les trois couches à mélanger
/// (`sampleBlend`). La foulée dure la moyenne des durées de la marche et de la course, pondérée par
/// leurs poids : elle s'allonge ou se raccourcit sans à-coup quand la vitesse change.
[[nodiscard]] std::array<ClipLayer, 3> advanceLocomotion(const AnimationSet& set,
                                                         const Locomotion& locomotion,
                                                         LocomotionClock& clock, float speed,
                                                         float seconds);

} // namespace levain::animation
