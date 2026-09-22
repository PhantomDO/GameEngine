#pragma once

#include <cstdint>

#include <glm/glm.hpp>
#include <nvrhi/nvrhi.h>

#include "levain/core/error.hpp"
#include "levain/render/mesh.hpp"

namespace levain::render
{

/// Le format du depth buffer : 32 bits flottants, disponible partout.
inline constexpr nvrhi::Format DepthFormat = nvrhi::Format::D32;

/// Les constantes du shader. Doit correspondre à `SceneConstants` dans `shaders/mesh.slang`.
struct SceneConstants
{
    glm::mat4 viewProjection;
    glm::mat4 model;
};

/// Dessine des meshes indexés avec un depth buffer. Tout est créé une fois ; chaque dessin ne fait
/// qu'écrire les constantes et enregistrer un draw.
struct MeshPass
{
    nvrhi::ShaderHandle vertexShader;
    nvrhi::ShaderHandle pixelShader;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::BindingLayoutHandle frameLayout;
    nvrhi::BindingLayoutHandle materialLayout;
    nvrhi::BufferHandle sceneConstants;
    nvrhi::BindingSetHandle frameBindings;
    nvrhi::GraphicsPipelineHandle pipeline;
};

/// Crée la passe pour des framebuffers de ce format, couleur et profondeur (`DepthFormat`).
[[nodiscard]] core::Result<MeshPass> createMeshPass(nvrhi::IDevice& device,
                                                    const nvrhi::FramebufferInfo& target);

/// Le nom des sources de la passe dans `shaders/`, sans extension : le hot-reload recrée la passe
/// quand ce fichier change (ADR-0014).
inline constexpr const char* MeshPassShaderFile = "mesh";

/// Recharge les shaders compilés de la passe et recrée son pipeline, sans toucher aux layouts, aux
/// buffers ni aux binding sets. En cas d'échec, `pass` reste tel quel (#46).
[[nodiscard]] core::Result<void> reloadMeshPassShaders(nvrhi::IDevice& device, MeshPass& pass,
                                                       const nvrhi::FramebufferInfo& target);

/// Le depth buffer à la taille de l'image où l'on dessine : recréé seulement quand elle change.
[[nodiscard]] nvrhi::ITexture* ensureDepthTexture(nvrhi::IDevice& device,
                                                  nvrhi::TextureHandle& depth, std::uint32_t width,
                                                  std::uint32_t height);

/// Le binding set d'un matériau (`space2`, ADR-0013) : sa texture et son sampler. Créé une fois par
/// matériau, pas à chaque dessin.
[[nodiscard]] nvrhi::BindingSetHandle createMaterialBindings(nvrhi::IDevice& device,
                                                             const MeshPass& pass,
                                                             nvrhi::ITexture& albedo,
                                                             nvrhi::ISampler& sampler);

/// Enregistre le dessin de toutes les `instances` de `mesh`, en un seul appel, dans `framebuffer`,
/// qui doit avoir un depth buffer.
void drawMesh(nvrhi::ICommandList& commandList, const MeshPass& pass,
              nvrhi::IFramebuffer& framebuffer, const Mesh& mesh, const Instances& instances,
              nvrhi::IBindingSet& material, const SceneConstants& constants);

} // namespace levain::render
