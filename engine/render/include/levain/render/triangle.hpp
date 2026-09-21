#pragma once

#include <nvrhi/nvrhi.h>

#include "levain/core/error.hpp"

namespace levain::render
{

/// Le premier triangle (M1.3) : deux shaders et le pipeline qui les relie. Aucune ressource : les
/// sommets sont écrits dans le shader (`shaders/triangle.slang`).
struct TrianglePass
{
    nvrhi::ShaderHandle vertexShader;
    nvrhi::ShaderHandle pixelShader;
    nvrhi::GraphicsPipelineHandle pipeline;
};

/// Charge les shaders compilés au build, au format du device (SPIR-V sous Vulkan, DXIL sous
/// Direct3D 12), et crée le pipeline pour des framebuffers de ce format.
[[nodiscard]] core::Result<TrianglePass> createTrianglePass(nvrhi::IDevice& device,
                                                            const nvrhi::FramebufferInfo& target);

/// Enregistre le dessin du triangle dans `framebuffer`, sur toute sa surface.
void drawTriangle(nvrhi::ICommandList& commandList, const TrianglePass& pass,
                  nvrhi::IFramebuffer& framebuffer);

} // namespace levain::render
