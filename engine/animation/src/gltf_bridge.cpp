// La passerelle glTF vers ozz (ADR-0022). Le traitement des clés STEP reprend celui de gltf2ozz
// (ozz-animation, src/animation/offline/gltf/gltf2ozz.cc, licence MIT, © Guillaume Blanc).

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/glm.hpp>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>

#include "ozz_data.hpp"

#include "levain/animation/animation_set.hpp"

namespace levain::animation
{

namespace
{

namespace offline = ozz::animation::offline;

std::unexpected<core::Error> bridgeError(const std::filesystem::path& path, std::string_view what)
{
    return core::makeError(core::ErrorCode::InvalidData,
                           std::format("{} : {}", path.string(), what));
}

ozz::math::Float3 toOzz(glm::vec3 value)
{
    return {value.x, value.y, value.z};
}

/// glTF range un quaternion en (x, y, z, w), comme ozz.
ozz::math::Quaternion toOzz(glm::vec4 value)
{
    return {value.x, value.y, value.z, value.w};
}

/// La transformation de repos d'un nœud. `DecomposeNodeMatrices` garantit la forme TRS.
ozz::math::Transform restTransformOf(const fastgltf::Node& node)
{
    const auto& trs = std::get<fastgltf::TRS>(node.transform);
    ozz::math::Transform transform{};
    transform.translation = {trs.translation.x(), trs.translation.y(), trs.translation.z()};
    transform.rotation = {trs.rotation.x(), trs.rotation.y(), trs.rotation.z(), trs.rotation.w()};
    transform.scale = {trs.scale.x(), trs.scale.y(), trs.scale.z()};
    return transform;
}

/// Le nom de chaque os, par indice de nœud, unique : ozz et la pose désignent un os par son nom.
std::map<std::size_t, std::string> uniqueJointNames(const fastgltf::Asset& asset,
                                                    const fastgltf::Skin& skin)
{
    std::map<std::size_t, std::string> names;
    std::set<std::string> taken;
    for (const std::size_t node : skin.joints)
    {
        std::string name{asset.nodes[node].name};
        if (name.empty() || taken.contains(name))
        {
            name = std::format("os#{}", node);
        }
        taken.insert(name);
        names.emplace(node, std::move(name));
    }
    return names;
}

/// Le parent de chaque nœud, s'il en a un : glTF ne donne que les enfants.
std::vector<std::optional<std::size_t>> parentsOf(const fastgltf::Asset& asset)
{
    std::vector<std::optional<std::size_t>> parents(asset.nodes.size());
    for (std::size_t node = 0; node < asset.nodes.size(); ++node)
    {
        for (const std::size_t child : asset.nodes[node].children)
        {
            parents[child] = node;
        }
    }
    return parents;
}

/// Ajoute l'os `node` à `siblings`, puis ses enfants qui sont des os.
void appendJoint(const fastgltf::Asset& asset, std::size_t node,
                 const std::map<std::size_t, std::string>& names,
                 offline::RawSkeleton::Joint::Children& siblings)
{
    offline::RawSkeleton::Joint& joint = siblings.emplace_back();
    joint.name = names.at(node).c_str();
    joint.transform = restTransformOf(asset.nodes[node]);
    for (const std::size_t child : asset.nodes[node].children)
    {
        if (names.contains(child))
        {
            appendJoint(asset, child, names, joint.children);
        }
    }
}

/// Le squelette d'ozz : les os dont le parent n'est pas un os en sont les racines. Un nœud qui
/// n'est pas un os, placé entre deux os, perdrait sa transformation : c'est un échec.
core::Result<offline::RawSkeleton> rawSkeletonOf(const fastgltf::Asset& asset,
                                                 const std::map<std::size_t, std::string>& names,
                                                 const std::filesystem::path& path)
{
    const std::vector<std::optional<std::size_t>> parents = parentsOf(asset);
    offline::RawSkeleton skeleton;
    for (const auto& [node, name] : names)
    {
        std::optional<std::size_t> parent = parents[node];
        if (parent && names.contains(*parent))
        {
            continue; // ajouté avec son parent
        }
        for (std::optional<std::size_t> ancestor = parent; ancestor; ancestor = parents[*ancestor])
        {
            if (names.contains(*ancestor))
            {
                return bridgeError(path, std::format("l'os « {} » est séparé de son os parent par "
                                                     "un nœud qui n'est pas un os",
                                                     name));
            }
        }
        appendJoint(asset, node, names, skeleton.roots);
    }
    return skeleton;
}

/// Les clés d'un canal, converties par `convert`. Une clé STEP devient deux clés, la seconde juste
/// avant la suivante : ozz n'interpole que linéairement, et le palier est ainsi tenu.
template <typename GlmValue, typename Key, typename Convert>
core::Result<void> readKeys(const fastgltf::Asset& asset, const fastgltf::AnimationSampler& sampler,
                            ozz::vector<Key>& keys, float& duration, Convert convert,
                            const std::filesystem::path& path)
{
    if (sampler.interpolation == fastgltf::AnimationInterpolation::CubicSpline)
    {
        return bridgeError(path, "clé CUBICSPLINE non prise en charge");
    }
    const fastgltf::Accessor& times = asset.accessors[sampler.inputAccessor];
    const fastgltf::Accessor& values = asset.accessors[sampler.outputAccessor];
    if (times.count != values.count || times.count == 0)
    {
        return bridgeError(path, "un canal d'animation a autant de temps que de valeurs, et au "
                                 "moins une clé");
    }
    std::vector<float> seconds(times.count);
    fastgltf::copyFromAccessor<float>(asset, times, seconds.data());
    const bool step = sampler.interpolation == fastgltf::AnimationInterpolation::Step;
    keys.clear();
    fastgltf::iterateAccessorWithIndex<GlmValue>(
        asset, values,
        [&](GlmValue value, std::size_t i)
        {
            keys.push_back({.time = seconds[i], .value = convert(value)});
            if (step && i + 1 < seconds.size())
            {
                keys.push_back(
                    {.time = std::nextafter(seconds[i + 1], 0.0f), .value = convert(value)});
            }
        });
    duration = std::max(duration, seconds.back());
    return {};
}

/// Un clip glTF, au format d'import d'ozz : une piste par os, dans l'ordre du squelette. Un os que
/// le clip n'anime pas garde sa pose de repos.
core::Result<offline::RawAnimation> rawAnimationOf(const fastgltf::Asset& asset,
                                                   const fastgltf::Animation& animation,
                                                   const ozz::animation::Skeleton& skeleton,
                                                   const std::map<std::string, std::size_t>& nodeOf,
                                                   const std::filesystem::path& path)
{
    offline::RawAnimation raw;
    raw.name = std::string{animation.name}.c_str();
    raw.duration = 0.0f;
    raw.tracks.resize(static_cast<std::size_t>(skeleton.num_joints()));
    for (std::size_t joint = 0; joint < raw.tracks.size(); ++joint)
    {
        const std::size_t node = nodeOf.at(skeleton.joint_names()[joint]);
        offline::RawAnimation::JointTrack& track = raw.tracks[joint];
        for (const fastgltf::AnimationChannel& channel : animation.channels)
        {
            if (!channel.nodeIndex || *channel.nodeIndex != node)
            {
                continue;
            }
            const fastgltf::AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
            const auto float3 = [](glm::vec3 value) { return toOzz(value); };
            const auto quaternion = [](glm::vec4 value) { return toOzz(glm::normalize(value)); };
            core::Result<void> read{};
            switch (channel.path)
            {
            case fastgltf::AnimationPath::Translation:
                read = readKeys<glm::vec3>(asset, sampler, track.translations, raw.duration, float3,
                                           path);
                break;
            case fastgltf::AnimationPath::Rotation:
                read = readKeys<glm::vec4>(asset, sampler, track.rotations, raw.duration,
                                           quaternion, path);
                break;
            case fastgltf::AnimationPath::Scale:
                read =
                    readKeys<glm::vec3>(asset, sampler, track.scales, raw.duration, float3, path);
                break;
            case fastgltf::AnimationPath::Weights:
                break; // les morph targets ne sont pas des os
            }
            if (!read)
            {
                return std::unexpected(read.error());
            }
        }
        const ozz::math::Transform rest = restTransformOf(asset.nodes[node]);
        if (track.translations.empty())
        {
            track.translations.push_back({.time = 0.0f, .value = rest.translation});
        }
        if (track.rotations.empty())
        {
            track.rotations.push_back({.time = 0.0f, .value = rest.rotation});
        }
        if (track.scales.empty())
        {
            track.scales.push_back({.time = 0.0f, .value = rest.scale});
        }
    }
    // ozz exige une durée positive. Un clip d'une seule clé (une pose fixe) n'en a pas : une
    // seconde, la valeur par défaut d'ozz, le tient.
    if (raw.duration <= 0.0f)
    {
        raw.duration = 1.0f;
    }
    return raw;
}

} // namespace

core::Result<AnimationSet> importAnimationSet(const std::filesystem::path& gltf)
{
    auto data = fastgltf::GltfDataBuffer::FromPath(gltf);
    if (data.error() != fastgltf::Error::None)
    {
        return core::makeError(
            core::ErrorCode::FileNotFound,
            std::format("{} : {}", gltf.string(), fastgltf::getErrorMessage(data.error())));
    }
    fastgltf::Parser parser;
    auto asset = parser.loadGltf(data.get(), gltf.parent_path(),
                                 fastgltf::Options::LoadExternalBuffers |
                                     fastgltf::Options::DecomposeNodeMatrices);
    if (asset.error() != fastgltf::Error::None)
    {
        return bridgeError(gltf, fastgltf::getErrorMessage(asset.error()));
    }
    if (asset->skins.empty())
    {
        return bridgeError(gltf, "aucun skin : rien à animer");
    }

    const std::map<std::size_t, std::string> names = uniqueJointNames(asset.get(), asset->skins[0]);
    auto rawSkeleton = rawSkeletonOf(asset.get(), names, gltf);
    if (!rawSkeleton)
    {
        return std::unexpected(rawSkeleton.error());
    }
    auto ozzData = std::make_shared<OzzData>();
    ozzData->skeleton = offline::SkeletonBuilder{}(*rawSkeleton);
    if (!ozzData->skeleton)
    {
        return bridgeError(gltf, "squelette refusé par ozz");
    }

    AnimationSet set;
    std::map<std::string, std::size_t> nodeOf;
    for (const auto& [node, name] : names)
    {
        nodeOf.emplace(name, node);
    }
    for (const char* name : ozzData->skeleton->joint_names())
    {
        set.jointNames.emplace_back(name);
    }
    for (const fastgltf::Animation& animation : asset->animations)
    {
        auto raw = rawAnimationOf(asset.get(), animation, *ozzData->skeleton, nodeOf, gltf);
        if (!raw)
        {
            return std::unexpected(raw.error());
        }
        auto built = offline::AnimationBuilder{}(*raw);
        if (!built)
        {
            return bridgeError(gltf, std::format("clip « {} » refusé par ozz", animation.name));
        }
        set.clips.push_back(
            {.name = std::string{animation.name}, .durationSeconds = raw->duration});
        ozzData->clips.push_back(std::move(built));
    }
    set.ozz = std::move(ozzData);
    return set;
}

} // namespace levain::animation
