#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "levain/core/error.hpp"

namespace levain::animation
{

/// Le squelette au format d'ozz. Défini dans `src/` : aucun type d'ozz ne sort du
/// module (ADR-0022).
struct OzzData;

/// Le squelette d'un glTF skinné, au format d'ozz. Ses données ne changent plus après l'import :
/// plusieurs personnages peuvent partager le même.
struct AnimationSet
{
    /// Dans l'ordre d'ozz, qui est celui de la pose : un parent avant ses enfants. Un nom glTF
    /// vide ou en double reçoit l'indice de son nœud (« os#12 »), pour rester unique.
    std::vector<std::string> jointNames;
    std::shared_ptr<const OzzData> ozz;
};

/// La passerelle glTF (ADR-0022) : lit avec fastgltf le premier skin d'un glTF, remplit les
/// structures d'import d'ozz, et les convertit à son format d'exécution. Un glTF sans skin, ou un
/// nœud qui n'est pas un os placé entre deux os, sont des échecs récupérables (ADR-0008).
[[nodiscard]] core::Result<AnimationSet> importAnimationSet(const std::filesystem::path& gltf);

} // namespace levain::animation
