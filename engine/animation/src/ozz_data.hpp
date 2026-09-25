#pragma once

#include <vector>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/memory/unique_ptr.h>

namespace levain::animation
{

/// Ce qu'`AnimationSet` cache : le squelette et les clips au format d'exécution d'ozz.
struct OzzData
{
    ozz::unique_ptr<ozz::animation::Skeleton> skeleton;
    std::vector<ozz::unique_ptr<ozz::animation::Animation>> clips;
};

} // namespace levain::animation
