#include "levain/animation/animator.hpp"

#include <algorithm>

namespace levain::animation
{

namespace
{

/// Le clip propre à un état, s'il en a un. Le sol n'en a pas : c'est la locomotion.
std::optional<std::size_t> clipOf(const AnimatorClips& clips, MotionState state)
{
    switch (state)
    {
    case MotionState::Ground:
        return std::nullopt;
    case MotionState::Jump:
        return clips.jump;
    case MotionState::Fall:
        return clips.fall;
    case MotionState::Swim:
        return clips.swim;
    case MotionState::Glide:
        return clips.glide;
    }
    return std::nullopt;
}

/// Les couches d'un état, à pleine valeur : son clip en boucle depuis `seconds`, ou, s'il n'en a
/// pas, les couches de la locomotion déjà avancées pour cette image.
std::array<ClipLayer, 3> layersOf(const AnimationSet& set, const AnimatorClips& clips,
                                  MotionState state, float seconds,
                                  const std::array<ClipLayer, 3>& ground)
{
    const std::optional<std::size_t> clip = clipOf(clips, state);
    if (!clip)
    {
        return ground;
    }
    // ponytail: le saut et la chute bouclent comme les autres ; les jouer une fois, puis tenir
    // leur dernière pose, quand Rando aura de vrais sauts (M6).
    return {ClipLayer{.clip = *clip,
                      .ratio = loopedRatio(seconds, set.clips[*clip].durationSeconds),
                      .weight = 1.0f},
            ClipLayer{.clip = *clip, .ratio = 0.0f, .weight = 0.0f},
            ClipLayer{.clip = *clip, .ratio = 0.0f, .weight = 0.0f}};
}

/// L'avancement du fondu après `seconds`, borné à 1. Une durée nulle termine le fondu aussitôt,
/// plutôt que de diviser par zéro.
float advanceFade(float fade, float seconds, float fadeSeconds)
{
    return fadeSeconds > 0.0f ? std::min(1.0f, fade + seconds / fadeSeconds) : 1.0f;
}

} // namespace

MotionState chooseState(const CharacterMotion& motion)
{
    if (motion.swimming)
    {
        return MotionState::Swim;
    }
    if (motion.gliding)
    {
        return MotionState::Glide;
    }
    if (motion.grounded)
    {
        return MotionState::Ground;
    }
    return motion.verticalSpeed > 0.0f ? MotionState::Jump : MotionState::Fall;
}

AnimatorLayers advanceAnimator(const AnimationSet& set, const AnimatorClips& clips,
                               Animator& animator, const CharacterMotion& motion, float seconds)
{
    // ponytail: un changement d'état pendant un fondu repart de l'état courant, et abandonne celui
    // qu'on quittait déjà ; garder une pile de fondus si ce saut se voit.
    const MotionState wanted = chooseState(motion);
    if (wanted != animator.state)
    {
        animator.previous = animator.state;
        animator.previousSeconds = animator.stateSeconds;
        animator.state = wanted;
        animator.stateSeconds = 0.0f;
        animator.fade = 0.0f;
    }
    animator.stateSeconds += seconds;
    animator.previousSeconds += seconds;
    animator.fade = advanceFade(animator.fade, seconds, animator.fadeSeconds);

    // La locomotion avance une seule fois par image, même si les deux états s'en servent.
    const std::array<ClipLayer, 3> ground =
        advanceLocomotion(set, clips.ground, animator.ground, motion.speed, seconds);
    const std::array<ClipLayer, 3> current =
        layersOf(set, clips, animator.state, animator.stateSeconds, ground);
    const std::array<ClipLayer, 3> previous =
        layersOf(set, clips, animator.previous, animator.previousSeconds, ground);

    AnimatorLayers layers{};
    for (std::size_t i = 0; i < current.size(); ++i)
    {
        layers[i] = current[i];
        layers[i].weight *= animator.fade;
        layers[current.size() + i] = previous[i];
        layers[current.size() + i].weight *= 1.0f - animator.fade;
    }
    return layers;
}

} // namespace levain::animation
