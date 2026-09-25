#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "levain/core/error.hpp"

namespace levain::animation
{

/// Le squelette et les clips au format d'ozz. Défini dans `src/` : aucun type d'ozz ne sort du
/// module (ADR-0022).
struct OzzData;

/// Un clip, tel que le reste du moteur le voit.
struct ClipInfo
{
    std::string name;
    float durationSeconds = 0.0f;
};

/// Le squelette d'un glTF skinné et ses clips, prêts à échantillonner (`pose.hpp`). Les données
/// d'ozz ne changent plus après l'import : plusieurs personnages peuvent partager le même.
struct AnimationSet
{
    /// Dans l'ordre d'ozz, qui est celui de la pose : un parent avant ses enfants. Un nom glTF
    /// vide ou en double reçoit l'indice de son nœud (« os#12 »), pour rester unique.
    std::vector<std::string> jointNames;
    std::vector<ClipInfo> clips; ///< Dans l'ordre du glTF.
    std::shared_ptr<const OzzData> ozz;
};

/// La passerelle glTF (ADR-0022) : lit avec fastgltf le premier skin et tous les clips d'un glTF,
/// remplit les structures d'import d'ozz, et les convertit à son format d'exécution. Un glTF sans
/// skin, un nœud qui n'est pas un os entre deux os, ou un clip qu'ozz refuse, sont des échecs
/// récupérables (ADR-0008).
[[nodiscard]] core::Result<AnimationSet> importAnimationSet(const std::filesystem::path& gltf);

} // namespace levain::animation
