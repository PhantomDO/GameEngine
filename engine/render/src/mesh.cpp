#include "levain/render/mesh.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace levain::render
{

Mesh createMesh(nvrhi::IDevice& device, nvrhi::ICommandList& commandList,
                std::span<const MeshVertex> vertices, std::span<const std::uint32_t> indices)
{
    // keepInitialState : NVRHI remet les buffers dans l'état où le GPU les lit après chaque command
    // list qui les a touchés, ici celle de l'envoi.
    Mesh mesh{
        .vertexBuffer =
            device.createBuffer(nvrhi::BufferDesc()
                                    .setByteSize(vertices.size_bytes())
                                    .setIsVertexBuffer(true)
                                    .setInitialState(nvrhi::ResourceStates::VertexBuffer)
                                    .setKeepInitialState(true)
                                    .setDebugName("mesh : sommets")),
        .indexBuffer = device.createBuffer(nvrhi::BufferDesc()
                                               .setByteSize(indices.size_bytes())
                                               .setIsIndexBuffer(true)
                                               .setInitialState(nvrhi::ResourceStates::IndexBuffer)
                                               .setKeepInitialState(true)
                                               .setDebugName("mesh : indices")),
        .indexCount = static_cast<std::uint32_t>(indices.size()),
    };

    // writeBuffer passe par un buffer d'envoi interne à NVRHI, qui place aussi les barrières (E1,
    // §4).
    commandList.writeBuffer(mesh.vertexBuffer, vertices.data(), vertices.size_bytes());
    commandList.writeBuffer(mesh.indexBuffer, indices.data(), indices.size_bytes());
    return mesh;
}

Instances createInstances(nvrhi::IDevice& device, nvrhi::ICommandList& commandList,
                          std::span<const glm::vec3> offsets)
{
    Instances instances{
        .offsets = device.createBuffer(nvrhi::BufferDesc()
                                           .setByteSize(offsets.size_bytes())
                                           .setIsVertexBuffer(true)
                                           .setInitialState(nvrhi::ResourceStates::VertexBuffer)
                                           .setKeepInitialState(true)
                                           .setDebugName("instances : positions")),
        .count = static_cast<std::uint32_t>(offsets.size()),
        .capacity = static_cast<std::uint32_t>(offsets.size()),
    };
    commandList.writeBuffer(instances.offsets, offsets.data(), offsets.size_bytes());
    return instances;
}

void updateInstances(nvrhi::ICommandList& commandList, Instances& instances,
                     std::span<const glm::vec3> offsets)
{
    // ponytail: capacité fixe ; des entités créées depuis l'explorer au-delà ne s'affichent pas.
    // Un buffer recréé plus grand le jour où la scène grandit pour de bon.
    const std::span<const glm::vec3> drawn =
        offsets.first(std::min(offsets.size(), static_cast<std::size_t>(instances.capacity)));
    // writeBuffer se place dans la command list, avant le dessin qui lit ces positions : NVRHI met
    // la barrière entre les deux, et le GPU n'écrase pas ce qu'une frame précédente lit encore.
    commandList.writeBuffer(instances.offsets, drawn.data(), drawn.size_bytes());
    instances.count = static_cast<std::uint32_t>(drawn.size());
}

/// Les coordonnées de texture des quatre coins d'une face carrée, dans l'ordre où createCube et
/// createPlane les donnent : bas gauche, bas droite, haut droite, haut gauche.
constexpr std::array<glm::vec2, 4> FaceUvs{
    {{0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}}};

Mesh createCube(nvrhi::IDevice& device, nvrhi::ICommandList& commandList)
{
    // Les huit coins, nommés par le signe de x, y et z (n : −0,5 ; p : +0,5).
    const glm::vec3 nnn{-0.5f, -0.5f, -0.5f};
    const glm::vec3 nnp{-0.5f, -0.5f, 0.5f};
    const glm::vec3 npn{-0.5f, 0.5f, -0.5f};
    const glm::vec3 npp{-0.5f, 0.5f, 0.5f};
    const glm::vec3 pnn{0.5f, -0.5f, -0.5f};
    const glm::vec3 pnp{0.5f, -0.5f, 0.5f};
    const glm::vec3 ppn{0.5f, 0.5f, -0.5f};
    const glm::vec3 ppp{0.5f, 0.5f, 0.5f};

    const glm::vec3 red{1.0f, 0.25f, 0.25f};
    const glm::vec3 cyan{0.25f, 1.0f, 1.0f};
    const glm::vec3 green{0.25f, 1.0f, 0.25f};
    const glm::vec3 magenta{1.0f, 0.25f, 1.0f};
    const glm::vec3 blue{0.25f, 0.25f, 1.0f};
    const glm::vec3 yellow{1.0f, 1.0f, 0.25f};

    // Une face par ligne, ses quatre coins dans le sens trigonométrique vu de l'extérieur : c'est
    // ce sens qui fait d'une face une face avant (MeshPass élimine les faces arrière).
    std::array<MeshVertex, 24> vertices{{
        {pnp, red, {}},     {pnn, red, {}},     {ppn, red, {}},     {ppp, red, {}},     // +x
        {nnn, cyan, {}},    {nnp, cyan, {}},    {npp, cyan, {}},    {npn, cyan, {}},    // −x
        {npp, green, {}},   {ppp, green, {}},   {ppn, green, {}},   {npn, green, {}},   // +y
        {nnn, magenta, {}}, {pnn, magenta, {}}, {pnp, magenta, {}}, {nnp, magenta, {}}, // −y
        {nnp, blue, {}},    {pnp, blue, {}},    {ppp, blue, {}},    {npp, blue, {}},    // +z
        {pnn, yellow, {}},  {nnn, yellow, {}},  {npn, yellow, {}},  {ppn, yellow, {}},  // −z
    }};

    // Chaque face commence par ses deux coins du bas (ou, pour ±y, par un bord), dans le même
    // sens : les quatre coins de chaque face reçoivent les mêmes coordonnées de texture.
    for (std::size_t i = 0; i < vertices.size(); ++i)
    {
        vertices[i].uv = FaceUvs[i % FaceUvs.size()];
    }

    // Deux triangles par face : (0, 1, 2) et (0, 2, 3) sur ses quatre coins.
    std::array<std::uint32_t, 36> indices{};
    for (std::size_t face = 0; face < 6; ++face)
    {
        const auto first = static_cast<std::uint32_t>(face * 4);
        const std::array<std::uint32_t, 6> quad{first, first + 1, first + 2,
                                                first, first + 2, first + 3};
        std::ranges::copy(quad, indices.begin() + static_cast<std::ptrdiff_t>(face * 6));
    }
    return createMesh(device, commandList, vertices, indices);
}

Mesh createPlane(nvrhi::IDevice& device, nvrhi::ICommandList& commandList, float size,
                 float textureRepeat)
{
    const float h = size / 2.0f;
    const glm::vec3 white{1.0f};
    // Les coins dans le sens trigonométrique vu d'en haut, comme la face +y du cube.
    std::array<MeshVertex, 4> vertices{{
        {{-h, 0.0f, h}, white, {}},
        {{h, 0.0f, h}, white, {}},
        {{h, 0.0f, -h}, white, {}},
        {{-h, 0.0f, -h}, white, {}},
    }};
    // Au-delà de 1, le sampler en Wrap répète la texture : textureRepeat fois sur chaque côté.
    for (std::size_t i = 0; i < vertices.size(); ++i)
    {
        vertices[i].uv = FaceUvs[i] * textureRepeat;
    }
    const std::array<std::uint32_t, 6> indices{0, 1, 2, 0, 2, 3};
    return createMesh(device, commandList, vertices, indices);
}

} // namespace levain::render
