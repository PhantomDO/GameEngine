#include "levain/render/triangle.hpp"

#include <filesystem>
#include <format>
#include <string_view>
#include <utility>

#include "levain/core/file.hpp"

namespace levain::render
{

namespace
{

/// SPIR-V pour Vulkan, DXIL pour Direct3D 12 : shaders/CMakeLists.txt produit les deux. C'est la
/// seule ligne du module qui dépend de l'API graphique.
std::string_view shaderExtensionFor(nvrhi::GraphicsAPI api)
{
    return api == nvrhi::GraphicsAPI::VULKAN ? ".spv" : ".dxil";
}

// ponytail: chemin absolu du dossier de build, suffisant tant qu'on lance depuis le build. À
// remplacer par un chemin relatif à l'exécutable le jour où on distribue un binaire.
std::filesystem::path shaderPath(nvrhi::IDevice& device, std::string_view name)
{
    return std::filesystem::path{LEVAIN_SHADER_DIR} /
           std::format("{}{}", name, shaderExtensionFor(device.getGraphicsAPI()));
}

core::Result<nvrhi::ShaderHandle> loadShader(nvrhi::IDevice& device, std::string_view name,
                                             nvrhi::ShaderType type)
{
    auto bytecode = core::readFile(shaderPath(device, name));
    if (!bytecode)
    {
        return std::unexpected(std::move(bytecode.error()));
    }

    // slangc nomme « main » le point d'entrée de chaque fichier SPIR-V qu'il produit.
    nvrhi::ShaderHandle shader =
        device.createShader(nvrhi::ShaderDesc().setShaderType(type).setEntryName("main"),
                            bytecode->data(), bytecode->size());
    if (!shader)
    {
        return core::makeError(core::ErrorCode::InvalidData, std::format("shader {} refusé", name));
    }
    return shader;
}

} // namespace

core::Result<TrianglePass> createTrianglePass(nvrhi::IDevice& device,
                                              const nvrhi::FramebufferInfo& target)
{
    auto vertexShader = loadShader(device, "triangle.vertexMain", nvrhi::ShaderType::Vertex);
    auto pixelShader = loadShader(device, "triangle.fragmentMain", nvrhi::ShaderType::Pixel);
    if (!vertexShader || !pixelShader)
    {
        return std::unexpected(vertexShader ? pixelShader.error() : vertexShader.error());
    }

    nvrhi::GraphicsPipelineDesc desc;
    desc.primType = nvrhi::PrimitiveType::TriangleList;
    desc.VS = *vertexShader;
    desc.PS = *pixelShader;
    // Ni tampon de profondeur, ni faces à écarter : un seul triangle, vu de face ou non.
    desc.renderState.depthStencilState.depthTestEnable = false;
    desc.renderState.depthStencilState.depthWriteEnable = false;
    desc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;

    nvrhi::GraphicsPipelineHandle pipeline = device.createGraphicsPipeline(desc, target);
    if (!pipeline)
    {
        return core::makeError(core::ErrorCode::InvalidData, "pipeline du triangle refusé");
    }
    return TrianglePass{.vertexShader = std::move(*vertexShader),
                        .pixelShader = std::move(*pixelShader),
                        .pipeline = std::move(pipeline)};
}

void drawTriangle(nvrhi::ICommandList& commandList, const TrianglePass& pass,
                  nvrhi::IFramebuffer& framebuffer)
{
    nvrhi::GraphicsState state;
    state.pipeline = pass.pipeline;
    state.framebuffer = &framebuffer;
    state.viewport.addViewportAndScissorRect(framebuffer.getFramebufferInfo().getViewport());
    commandList.setGraphicsState(state);

    nvrhi::DrawArguments arguments;
    arguments.vertexCount = 3;
    commandList.draw(arguments);
}

} // namespace levain::render
