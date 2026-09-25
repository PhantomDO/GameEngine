#pragma once

#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/memory/unique_ptr.h>

namespace levain::animation
{

/// Ce qu'`AnimationSet` cache : le squelette au format d'exécution d'ozz.
struct OzzData
{
    ozz::unique_ptr<ozz::animation::Skeleton> skeleton;
};

} // namespace levain::animation
