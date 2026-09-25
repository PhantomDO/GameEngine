// La passerelle glTF vers ozz (ADR-0022).

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
#include <fastgltf/types.hpp>
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
    for (const char* name : ozzData->skeleton->joint_names())
    {
        set.jointNames.emplace_back(name);
    }
    set.ozz = std::move(ozzData);
    return set;
}

} // namespace levain::animation
