#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <flecs.h>
#include <glm/glm.hpp>

#include "levain/assets/asset_id.hpp"
#include "levain/assets/image.hpp"
#include "levain/core/error.hpp"
#include "levain/scene/components.hpp"

namespace levain::assets
{

/// Un sommet tel que glTF le décrit. Les normales et les coordonnées de texture sont facultatives
/// dans un glTF : absentes, elles valent (0, 1, 0) et (0, 0).
struct ModelVertex
{
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f}; ///< (0, 0) en haut à gauche, comme chez nous : aucune inversion.
};

/// Une *primitive* glTF : des triangles indexés, qui partagent un matériau. Un mesh glTF en a une
/// ou plusieurs.
struct MeshPrimitive
{
    std::vector<ModelVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::optional<std::uint32_t> material; ///< Indice dans `Model::materials`.
};

/// Ce qu'un matériau glTF dit de la couleur de base (M4.1). Le reste du modèle metallic-roughness
/// (rugosité, métal, normales) viendra avec le PBR, en M5.1.
struct ModelMaterial
{
    glm::vec4 baseColorFactor{1.0f}; ///< Multiplie la texture ; seul, si elle est absente.
    std::optional<std::uint32_t> baseColorImage; ///< Indice dans `Model::images`.
};

struct ModelMesh
{
    std::string name;
    std::vector<MeshPrimitive> primitives;
};

/// Un nœud de la scène glTF, **parent avant enfants** : `parent` désigne toujours un nœud déjà vu.
struct ModelNode
{
    std::string name;
    scene::Transform local; ///< Relatif au parent, comme le `Transform` du moteur.
    std::optional<std::uint32_t> mesh;
    std::optional<std::uint32_t> parent;
};

/// Un fichier glTF lu en mémoire, sans GPU ni monde flecs : il se teste seul, et la cuisson des
/// assets (M4.3) pourra repartir de lui.
struct Model
{
    std::vector<ModelMesh> meshes;
    std::vector<ModelNode> nodes;
    std::vector<ModelMaterial> materials;
    /// Les images de couleur de base, décodées en RGBA. Seules celles qu'un matériau utilise :
    /// les normal maps de Sponza, par exemple, ne servent qu'à partir de M5.1.
    std::vector<Image> images;
};

/// Lit un `.gltf` (et ses `.bin` et images) ou un `.glb`, avec fastgltf. Seule la scène par défaut
/// est gardée ; seuls les triangles sont acceptés. Un fichier illisible ou incomplet est un échec
/// récupérable (ADR-0008).
[[nodiscard]] core::Result<Model> loadGltf(const std::filesystem::path& path);

/// Crée une entité par nœud, avec son `Transform` et sa hiérarchie (`flecs::Parent`, ADR-0015),
/// sous une entité racine nommée `rootName`, que l'on déplace pour déplacer tout le modèle. Un
/// nœud qui porte un mesh reçoit un `MeshRef` vers le mesh de l'asset `asset` (ADR-0019) : le
/// modèle compte alors une référence de plus, et reste chargé tant qu'il en a.
///
/// Le monde doit avoir importé `AssetsModule` (`asset_ref.hpp`).
flecs::entity instantiateModel(flecs::world& world, const Model& model, AssetId asset,
                               std::string_view rootName);

} // namespace levain::assets
