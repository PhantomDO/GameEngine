#pragma once

#include <cstdint>
#include <span>

#include <glm/glm.hpp>
#include <nvrhi/nvrhi.h>

namespace levain::render
{

/// Un sommet : position, couleur et coordonnées de texture. Doit correspondre à `VertexInput` dans
/// `shaders/mesh.slang`.
struct MeshVertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 uv; ///< (0, 0) en haut à gauche de la texture, comme sous Direct3D et Vulkan.
};

/// Un mesh indexé en mémoire GPU : chaque sommet n'est stocké qu'une fois, les triangles le
/// désignent par son indice. Un cube : 24 sommets au lieu de 36.
struct Mesh
{
    nvrhi::BufferHandle vertexBuffer;
    nvrhi::BufferHandle indexBuffer;
    std::uint32_t indexCount = 0;
};

/// Les copies d'un mesh dessinées en un seul appel (instancing) : une position par instance, lue
/// une fois par instance et non une fois par sommet. Doit correspondre à `instanceOffset` dans
/// `shaders/mesh.slang`.
struct Instances
{
    // ponytail: une instance n'est qu'un décalage, pas une matrice. Un parent qu'on ferait tourner
    // déplacerait donc ses enfants sans les tourner (M3.2). Passer aux matrices quand un objet de
    // la scène devra tourner ou changer d'échelle indépendamment du mesh.
    nvrhi::BufferHandle offsets;
    std::uint32_t count = 0;    ///< Instances dessinées.
    std::uint32_t capacity = 0; ///< Positions que le buffer peut contenir.
};

/// Crée le buffer des positions, à la taille de `offsets`, et enregistre son envoi dans
/// `commandList`.
[[nodiscard]] Instances createInstances(nvrhi::IDevice& device, nvrhi::ICommandList& commandList,
                                        std::span<const glm::vec3> offsets);

/// Remplace les positions des instances, par exemple à chaque frame depuis le monde flecs. Au-delà
/// de la capacité du buffer, les positions en trop ne sont pas dessinées.
void updateInstances(nvrhi::ICommandList& commandList, Instances& instances,
                     std::span<const glm::vec3> offsets);

/// Crée les buffers et enregistre l'envoi des données dans `commandList`, que l'appelant a ouverte
/// et exécutera avant le premier dessin.
[[nodiscard]] Mesh createMesh(nvrhi::IDevice& device, nvrhi::ICommandList& commandList,
                              std::span<const MeshVertex> vertices,
                              std::span<const std::uint32_t> indices);

/// Un cube de côté 1, centré sur l'origine, une couleur par face, la texture entière sur chaque
/// face.
[[nodiscard]] Mesh createCube(nvrhi::IDevice& device, nvrhi::ICommandList& commandList);

/// Un sol carré de côté `size`, horizontal, centré sur l'origine et tourné vers le haut, sur lequel
/// la texture se répète `textureRepeat` fois dans chaque direction.
[[nodiscard]] Mesh createPlane(nvrhi::IDevice& device, nvrhi::ICommandList& commandList, float size,
                               float textureRepeat);

} // namespace levain::render
