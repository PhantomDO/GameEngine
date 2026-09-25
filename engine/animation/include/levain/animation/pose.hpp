#pragma once

#include <cstddef>
#include <span>
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

/// Un clip dans un mélange : où il en est, en fraction de sa durée (0 au début, 1 à la fin), et
/// son poids. ozz normalise les poids : seuls leurs rapports comptent.
struct ClipLayer
{
    std::size_t clip = 0;
    float ratio = 0.0f;
    float weight = 1.0f;
};

/// La position dans un clip qui boucle, en fraction de sa durée : ozz échantillonne par ratio,
/// entre 0 et 1, et non en secondes.
[[nodiscard]] float loopedRatio(float seconds, float duration);

/// Échantillonne chaque couche à son ratio, les mélange selon leurs poids, et écrit la pose. Une
/// couche de poids nul n'est pas échantillonnée ; sans aucune couche, c'est la pose de repos.
void sampleBlend(const AnimationSet& set, std::span<const ClipLayer> layers, Pose& pose);

/// Échantillonne le clip `clip` de `set` à `seconds`, en boucle, et écrit la pose. Un indice de
/// clip hors limites est un bug de l'appelant.
void samplePose(const AnimationSet& set, std::size_t clip, float seconds, Pose& pose);

/// Les matrices du skinning, une par os du skin, dans l'ordre de ses indices de sommets : chacune
/// amène un sommet de sa place au repos à sa place dans `pose`, dans le repère du modèle. Au repos,
/// ce sont des identités.
void skinningMatrices(const AnimationSet& set, const Pose& pose, std::vector<glm::mat4>& matrices);

} // namespace levain::animation
