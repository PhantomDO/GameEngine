#include "levain/assets/gltf.hpp"

#include <format>
#include <numeric>
#include <span>
#include <string>
#include <variant>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include "levain/assets/asset_ref.hpp"

namespace levain::assets
{

namespace
{

std::unexpected<core::Error> gltfError(const std::filesystem::path& path, std::string_view what)
{
    return core::makeError(core::ErrorCode::InvalidData,
                           std::format("{} : {}", path.string(), what));
}

/// Remplit `vertices[i].*field` avec l'attribut `name`, s'il existe. Rend faux si sa taille ne
/// correspond pas aux positions : un glTF mal formé, que fastgltf ne vérifie pas.
template <typename Element, typename Field>
bool readAttribute(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                   std::string_view name, std::vector<ModelVertex>& vertices,
                   Field ModelVertex::* field)
{
    const auto* attribute = primitive.findAttribute(name);
    if (attribute == primitive.attributes.end())
    {
        return true;
    }
    const fastgltf::Accessor& accessor = asset.accessors[attribute->accessorIndex];
    if (accessor.count != vertices.size())
    {
        return false;
    }
    fastgltf::iterateAccessorWithIndex<Element>(asset, accessor, [&](Element value, std::size_t i)
                                                { vertices[i].*field = value; });
    return true;
}

core::Result<MeshPrimitive> readPrimitive(const fastgltf::Asset& asset,
                                          const fastgltf::Primitive& primitive,
                                          const std::filesystem::path& path)
{
    if (primitive.type != fastgltf::PrimitiveType::Triangles)
    {
        return gltfError(path, "seuls les triangles sont pris en charge");
    }
    const auto* position = primitive.findAttribute("POSITION");
    if (position == primitive.attributes.end())
    {
        return gltfError(path, "une primitive sans POSITION");
    }

    MeshPrimitive result;
    result.material = primitive.materialIndex
                          ? std::optional{static_cast<std::uint32_t>(*primitive.materialIndex)}
                          : std::nullopt;
    const fastgltf::Accessor& positions = asset.accessors[position->accessorIndex];
    result.vertices.resize(positions.count);
    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, positions,
                                                  [&](glm::vec3 value, std::size_t i)
                                                  { result.vertices[i].position = value; });
    if (!readAttribute<glm::vec3>(asset, primitive, "NORMAL", result.vertices,
                                  &ModelVertex::normal) ||
        !readAttribute<glm::vec2>(asset, primitive, "TEXCOORD_0", result.vertices,
                                  &ModelVertex::uv))
    {
        return gltfError(path, "un attribut n'a pas autant d'éléments que POSITION");
    }

    // Sans indices, les sommets se lisent trois par trois : 0, 1, 2, puis 3, 4, 5…
    if (primitive.indicesAccessor)
    {
        const fastgltf::Accessor& indices = asset.accessors[*primitive.indicesAccessor];
        result.indices.resize(indices.count);
        fastgltf::copyFromAccessor<std::uint32_t>(asset, indices, result.indices.data());
    }
    else
    {
        result.indices.resize(result.vertices.size());
        std::iota(result.indices.begin(), result.indices.end(), 0u);
    }
    return result;
}

/// Décode une image glTF, où qu'elle soit : un fichier à côté du `.gltf`, des octets embarqués
/// (base64), ou une portion d'un buffer (le cas des `.glb`).
core::Result<Image> readImage(const fastgltf::Asset& asset, const fastgltf::Image& image,
                              const std::filesystem::path& path)
{
    const std::string name = std::format("{} : image « {} »", path.string(), image.name);
    return std::visit(
        fastgltf::visitor{
            [&](const fastgltf::sources::URI& uri) -> core::Result<Image>
            {
                if (!uri.uri.isLocalPath())
                {
                    return gltfError(path, "image hors du disque (URI distante)");
                }
                return loadImage(path.parent_path() / uri.uri.fspath());
            },
            [&](const fastgltf::sources::Array& array) -> core::Result<Image>
            { return decodeImage(std::span{array.bytes.data(), array.bytes.size()}, name); },
            [&](const fastgltf::sources::BufferView& view) -> core::Result<Image>
            {
                const fastgltf::BufferView& bufferView = asset.bufferViews[view.bufferViewIndex];
                const auto* bytes = std::get_if<fastgltf::sources::Array>(
                    &asset.buffers[bufferView.bufferIndex].data);
                if (bytes == nullptr)
                {
                    return gltfError(path, "image dans un buffer non chargé");
                }
                return decodeImage(
                    std::span{bytes->bytes.data() + bufferView.byteOffset, bufferView.byteLength},
                    name);
            },
            [&](const auto&) -> core::Result<Image>
            { return gltfError(path, "source d'image non prise en charge"); }},
        image.data);
}

/// Les matériaux, et les seules images qu'ils utilisent comme couleur de base, renumérotées dans
/// l'ordre de première utilisation.
core::Result<void> readMaterials(const fastgltf::Asset& asset, const std::filesystem::path& path,
                                 Model& model)
{
    std::vector<std::optional<std::uint32_t>> imageSlot(asset.images.size());
    for (const fastgltf::Material& material : asset.materials)
    {
        const auto& factor = material.pbrData.baseColorFactor;
        ModelMaterial& out = model.materials.emplace_back(
            ModelMaterial{.baseColorFactor = {factor.x(), factor.y(), factor.z(), factor.w()},
                          .baseColorImage = std::nullopt});
        const auto& texture = material.pbrData.baseColorTexture;
        if (!texture || !asset.textures[texture->textureIndex].imageIndex)
        {
            continue;
        }
        const std::size_t source = *asset.textures[texture->textureIndex].imageIndex;
        if (!imageSlot[source])
        {
            auto image = readImage(asset, asset.images[source], path);
            if (!image)
            {
                return std::unexpected(image.error());
            }
            imageSlot[source] = static_cast<std::uint32_t>(model.images.size());
            model.images.push_back(std::move(*image));
        }
        out.baseColorImage = imageSlot[source];
    }
    return {};
}

/// Le `Transform` d'un nœud. `DecomposeNodeMatrices` garantit la forme TRS : une matrice glTF ne
/// peut ni cisailler ni projeter, elle se décompose toujours.
scene::Transform localTransformOf(const fastgltf::Node& node)
{
    const auto& trs = std::get<fastgltf::TRS>(node.transform);
    // fastgltf range un quaternion en (x, y, z, w), glm le construit en (w, x, y, z).
    return scene::Transform{
        .position = {trs.translation.x(), trs.translation.y(), trs.translation.z()},
        .rotation =
            glm::quat{trs.rotation.w(), trs.rotation.x(), trs.rotation.y(), trs.rotation.z()},
        .scale = {trs.scale.x(), trs.scale.y(), trs.scale.z()}};
}

/// Ajoute `nodeIndex` puis ses descendants, parent d'abord : l'ordre que promet `Model::nodes`.
void appendNode(const fastgltf::Asset& asset, std::size_t nodeIndex,
                std::optional<std::uint32_t> parent, Model& model)
{
    const fastgltf::Node& node = asset.nodes[nodeIndex];
    const auto self = static_cast<std::uint32_t>(model.nodes.size());
    model.nodes.push_back(ModelNode{
        .name = std::string{node.name},
        .local = localTransformOf(node),
        .mesh = node.meshIndex ? std::optional{static_cast<std::uint32_t>(*node.meshIndex)}
                               : std::nullopt,
        .parent = parent});
    for (const std::size_t child : node.children)
    {
        appendNode(asset, child, self, model);
    }
}

} // namespace

core::Result<Model> loadGltf(const std::filesystem::path& path)
{
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (data.error() != fastgltf::Error::None)
    {
        return core::makeError(
            core::ErrorCode::FileNotFound,
            std::format("{} : {}", path.string(), fastgltf::getErrorMessage(data.error())));
    }

    fastgltf::Parser parser;
    auto asset = parser.loadGltf(data.get(), path.parent_path(),
                                 fastgltf::Options::LoadExternalBuffers |
                                     fastgltf::Options::DecomposeNodeMatrices);
    if (asset.error() != fastgltf::Error::None)
    {
        return gltfError(path, fastgltf::getErrorMessage(asset.error()));
    }

    Model model;
    if (auto materials = readMaterials(asset.get(), path, model); !materials)
    {
        return std::unexpected(materials.error());
    }
    for (const fastgltf::Mesh& mesh : asset->meshes)
    {
        ModelMesh& out =
            model.meshes.emplace_back(ModelMesh{.name = std::string{mesh.name}, .primitives = {}});
        for (const fastgltf::Primitive& primitive : mesh.primitives)
        {
            auto read = readPrimitive(asset.get(), primitive, path);
            if (!read)
            {
                return std::unexpected(read.error());
            }
            out.primitives.push_back(std::move(*read));
        }
    }

    // La scène par défaut, ou la première ; un glTF sans scène n'a rien à montrer.
    const std::size_t sceneIndex = asset->defaultScene.value_or(0);
    if (sceneIndex >= asset->scenes.size())
    {
        return gltfError(path, "aucune scène");
    }
    for (const std::size_t root : asset->scenes[sceneIndex].nodeIndices)
    {
        appendNode(asset.get(), root, std::nullopt, model);
    }
    return model;
}

flecs::entity instantiateModel(flecs::world& world, const Model& model, AssetId asset,
                               std::string_view rootName)
{
    const flecs::entity root = world.entity(std::string{rootName}.c_str()).set(scene::Transform{});
    std::vector<flecs::entity> entities;
    entities.reserve(model.nodes.size());
    for (const ModelNode& node : model.nodes)
    {
        // Les nœuds sont rangés parent d'abord : l'entité du parent existe déjà. Les noms glTF
        // ne sont pas uniques ; une entité anonyme évite qu'un doublon en écrase un autre.
        const flecs::entity parent = node.parent ? entities[*node.parent] : root;
        flecs::entity entity = world.entity(flecs::Parent{parent}).set(node.local);
        if (node.mesh)
        {
            entity.set(MeshRef{.mesh = {.asset = asset, .sub = *node.mesh}});
        }
        entities.push_back(entity);
    }
    return root;
}

} // namespace levain::assets
