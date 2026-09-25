#include "levain/animation/locomotion.hpp"

#include <algorithm>

namespace levain::animation
{

std::array<float, 3> strideWeightsOf(const Locomotion& locomotion, float speed)
{
    if (speed <= 0.0f)
    {
        return {1.0f, 0.0f, 0.0f};
    }
    if (speed < locomotion.walkSpeed)
    {
        const float walk = speed / locomotion.walkSpeed;
        return {1.0f - walk, walk, 0.0f};
    }
    const float run = std::min(
        (speed - locomotion.walkSpeed) / (locomotion.runSpeed - locomotion.walkSpeed), 1.0f);
    return {0.0f, 1.0f - run, run};
}

std::array<ClipLayer, 3> advanceLocomotion(const AnimationSet& set, const Locomotion& locomotion,
                                           LocomotionClock& clock, float speed, float seconds)
{
    const std::array<float, 3> weights = strideWeightsOf(locomotion, speed);

    // La durée d'une foulée : celle de la marche et de la course, pondérées entre elles seules. À
    // l'arrêt, la phase ne bouge pas : le premier pas repartira d'où le dernier s'est arrêté.
    const float moving = weights[1] + weights[2];
    if (moving > 0.0f)
    {
        const float strideDuration = (weights[1] * set.clips[locomotion.walk].durationSeconds +
                                      weights[2] * set.clips[locomotion.run].durationSeconds) /
                                     moving;
        clock.stridePhase = loopedRatio(clock.stridePhase + seconds / strideDuration, 1.0f);
    }
    clock.idleSeconds += seconds;

    return {ClipLayer{
                .clip = locomotion.idle,
                .ratio = loopedRatio(clock.idleSeconds, set.clips[locomotion.idle].durationSeconds),
                .weight = weights[0]},
            ClipLayer{.clip = locomotion.walk, .ratio = clock.stridePhase, .weight = weights[1]},
            ClipLayer{.clip = locomotion.run, .ratio = clock.stridePhase, .weight = weights[2]}};
}

} // namespace levain::animation
