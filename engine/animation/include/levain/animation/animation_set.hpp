#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

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
    /// Le skin, dans l'ordre de ses os en glTF, qui est celui des indices d'os des sommets
    /// (`JOINTS_0`) : l'indice de chaque os dans la pose, et sa matrice de liaison inverse, qui
    /// ramène un sommet de sa place au repos dans le repère de l'os.
    std::vector<std::uint16_t> skinJoints;
    std::vector<glm::mat4> inverseBindMatrices;
    /// Les nœuds au-dessus des racines du squelette (le « porteur »), que la pose ne contient pas :
    /// ils placent le squelette dans le repère du modèle.
    glm::mat4 skeletonToModel{1.0f};
    std::shared_ptr<const OzzData> ozz;
};

/// La passerelle glTF (ADR-0022) : lit avec fastgltf le premier skin et tous les clips d'un glTF,
/// remplit les structures d'import d'ozz, et les convertit à son format d'exécution. Un glTF sans
/// skin, un nœud qui n'est pas un os entre deux os, ou un clip qu'ozz refuse, sont des échecs
/// récupérables (ADR-0008).
[[nodiscard]] core::Result<AnimationSet> importAnimationSet(const std::filesystem::path& gltf);

} // namespace levain::animation
