#pragma once

#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

#include "levain/animation/animation_set.hpp"

namespace levain::animation
{

/// La pose d'un personnage : la matrice de chaque os dans le repère du squelette, dans l'ordre de
/// `AnimationSet::jointNames`. C'est ce que lira le skinning (ADR-0022).
struct Pose
{
    std::vector<glm::mat4> joints;
};

/// Échantillonne le clip `clip` de `set` à `seconds`, en boucle, et écrit la pose. Un indice de
/// clip hors limites est un bug de l'appelant.
void samplePose(const AnimationSet& set, std::size_t clip, float seconds, Pose& pose);

/// Les matrices du skinning, une par os du skin, dans l'ordre de ses indices de sommets : chacune
/// amène un sommet de sa place au repos à sa place dans `pose`, dans le repère du modèle. Au repos,
/// ce sont des identités.
void skinningMatrices(const AnimationSet& set, const Pose& pose, std::vector<glm::mat4>& matrices);

} // namespace levain::animation
