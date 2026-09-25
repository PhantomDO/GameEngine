#include "levain/animation/pose.hpp"

#include <cmath>
#include <vector>

#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/span.h>

#include "ozz_data.hpp"

#include "levain/core/assert.hpp"

namespace levain::animation
{

namespace
{

/// La position dans un clip qui boucle, en fraction de sa durée : ozz échantillonne par ratio,
/// entre 0 et 1, et non en secondes.
float loopedRatio(float seconds, float duration)
{
    const float looped = std::fmod(seconds, duration);
    return (looped < 0.0f ? looped + duration : looped) / duration;
}

} // namespace

void samplePose(const AnimationSet& set, std::size_t clip, float seconds, Pose& pose)
{
    LEVAIN_ASSERT(clip < set.clips.size(), "indice de clip hors limites");
    const ozz::animation::Skeleton& skeleton = *set.ozz->skeleton;
    const ozz::animation::Animation& animation = *set.ozz->clips[clip];

    // ponytail: les tampons sont alloués à chaque appel ; les garder par personnage si la mesure
    // du coût CPU (#117) le demande. Le contexte d'ozz mémorise aussi où il en était dans le clip.
    ozz::animation::SamplingJob::Context context(animation.num_tracks());
    std::vector<ozz::math::SoaTransform> locals(
        static_cast<std::size_t>(skeleton.num_soa_joints()));
    std::vector<ozz::math::Float4x4> models(static_cast<std::size_t>(skeleton.num_joints()));

    // Deux jobs d'ozz (manuel d'ozz, « Sampling » et « Local to model ») : le clip donne la
    // transformation locale de chaque os, rangée par quatre os (SoA), puis la hiérarchie donne sa
    // matrice dans le repère du squelette.
    ozz::animation::SamplingJob sampling;
    sampling.animation = &animation;
    sampling.context = &context;
    sampling.ratio = loopedRatio(seconds, animation.duration());
    sampling.output = ozz::make_span(locals);
    LEVAIN_VERIFY(sampling.Run(), "échantillonnage refusé par ozz");

    ozz::animation::LocalToModelJob localToModel;
    localToModel.skeleton = &skeleton;
    localToModel.input = ozz::make_span(std::as_const(locals));
    localToModel.output = ozz::make_span(models);
    LEVAIN_VERIFY(localToModel.Run(), "calcul des matrices refusé par ozz");

    // ozz et glm rangent tous deux leurs matrices par colonnes.
    pose.joints.resize(models.size());
    for (std::size_t joint = 0; joint < models.size(); ++joint)
    {
        for (int column = 0; column < 4; ++column)
        {
            ozz::math::StorePtrU(models[joint].cols[column], &pose.joints[joint][column][0]);
        }
    }
}

void skinningMatrices(const AnimationSet& set, const Pose& pose, std::vector<glm::mat4>& matrices)
{
    matrices.resize(set.skinJoints.size());
    for (std::size_t joint = 0; joint < matrices.size(); ++joint)
    {
        matrices[joint] = set.skeletonToModel * pose.joints[set.skinJoints[joint]] *
                          set.inverseBindMatrices[joint];
    }
}

} // namespace levain::animation
