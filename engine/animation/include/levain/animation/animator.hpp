#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "levain/animation/animation_set.hpp"
#include "levain/animation/locomotion.hpp"
#include "levain/animation/pose.hpp"

namespace levain::animation
{

/// Ce que fait le personnage, vu de l'animation (#118, ADR-0022).
enum class MotionState : std::uint8_t
{
    Ground, ///< Repos, marche ou course, selon la vitesse (`Locomotion`).
    Jump,
    Fall,
    Swim,
    Glide,
};

/// Ce que le gameplay dit du personnage à chaque image. Le contrôleur de *Rando* le remplira (M6).
struct CharacterMotion
{
    float speed = 0.0f; ///< Horizontale, dans l'unité de `Locomotion`.
    bool grounded = true;
    float verticalSpeed = 0.0f; ///< Positive vers le haut.
    bool swimming = false;
    bool gliding = false;
};

/// L'état que demande `motion`. Dans l'ordre : l'eau, puis le vol plané, puis le sol ; en l'air, on
/// saute tant qu'on monte, et on tombe dès qu'on descend.
[[nodiscard]] MotionState chooseState(const CharacterMotion& motion);

/// Les clips du personnage : la locomotion au sol, et un clip par autre état. Un état sans clip
/// (Fox n'a ni saut ni nage) garde la locomotion au sol.
struct AnimatorClips
{
    Locomotion ground;
    std::optional<std::size_t> jump;
    std::optional<std::size_t> fall;
    std::optional<std::size_t> swim;
    std::optional<std::size_t> glide;
};

/// L'animateur d'un personnage : l'état courant, celui qu'il quitte, et l'avancement du fondu
/// entre les deux, de 0 (tout l'ancien) à 1 (tout le nouveau).
struct Animator
{
    MotionState state = MotionState::Ground;
    MotionState previous = MotionState::Ground;
    float fade = 1.0f;
    float fadeSeconds = 0.2f; ///< La durée d'un fondu entre deux états.
    LocomotionClock ground;
    float stateSeconds = 0.0f;    ///< Depuis l'entrée dans l'état courant.
    float previousSeconds = 0.0f; ///< Depuis l'entrée dans l'état quitté.
};

/// Les couches d'un mélange d'animateur : trois au plus pour l'état courant (la locomotion), trois
/// pour l'état quitté. Les cases inutiles ont un poids nul.
using AnimatorLayers = std::array<ClipLayer, 6>;

/// Avance l'animateur de `seconds` et rend les couches à mélanger (`sampleBlend`) : celles de
/// l'état courant pondérées par l'avancement du fondu, celles de l'état quitté par ce qui en reste.
/// Un changement d'état commence un nouveau fondu.
[[nodiscard]] AnimatorLayers advanceAnimator(const AnimationSet& set, const AnimatorClips& clips,
                                             Animator& animator, const CharacterMotion& motion,
                                             float seconds);

} // namespace levain::animation
