#include "levain/render/mesh_pass.hpp"

#include <array>
#include <cstddef>
#include <utility>

#include "shader.hpp"

namespace levain::render
{

core::Result<MeshPass> createMeshPass(nvrhi::IDevice& device, const nvrhi::FramebufferInfo& target)
{
    auto vertexShader = loadShader(device, "mesh.vertexMain", nvrhi::ShaderType::Vertex);
    auto pixelShader = loadShader(device, "mesh.fragmentMain", nvrhi::ShaderType::Pixel);
    if (!vertexShader || !pixelShader)
    {
        return std::unexpected(vertexShader ? pixelShader.error() : vertexShader.error());
    }

    // Les noms sont les sémantiques de VertexInput dans shaders/mesh.slang. Les deux premiers
    // viennent du buffer des sommets (slot 0), le troisième du buffer des instances (slot 1), lu
    // une fois par instance.
    const std::array<nvrhi::VertexAttributeDesc, 3> attributes{
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(offsetof(MeshVertex, position))
            .setElementStride(sizeof(MeshVertex)),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(offsetof(MeshVertex, color))
            .setElementStride(sizeof(MeshVertex)),
        nvrhi::VertexAttributeDesc()
            .setName("INSTANCE_OFFSET")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setBufferIndex(1)
            .setElementStride(sizeof(glm::vec3))
            .setIsInstanced(true),
    };
    nvrhi::InputLayoutHandle inputLayout =
        device.createInputLayout(attributes.data(), attributes.size(), *vertexShader);

    // ADR-0013 : un binding layout par fréquence de changement. Celui de la frame occupe space0,
    // qui devient le descriptor set 0 sous Vulkan.
    nvrhi::BindingLayoutDesc frameLayoutDesc;
    frameLayoutDesc.visibility = nvrhi::ShaderType::All;
    frameLayoutDesc.setRegisterSpaceAndDescriptorSet(0);
    frameLayoutDesc.bindings = {nvrhi::BindingLayoutItem::VolatileConstantBuffer(0)};
    nvrhi::BindingLayoutHandle frameLayout = device.createBindingLayout(frameLayoutDesc);

    // Volatile : le contenu ne vit que le temps d'une command list, et NVRHI en fournit une
    // nouvelle version à chaque écriture. Pas de buffer par frame en vol à gérer (QA du
    // 2026-09-21).
    nvrhi::BufferHandle sceneConstants =
        device.createBuffer(nvrhi::BufferDesc()
                                .setByteSize(sizeof(SceneConstants))
                                .setIsConstantBuffer(true)
                                .setIsVolatile(true)
                                .setMaxVersions(16)
                                .setDebugName("constantes de scène"));
    nvrhi::BindingSetHandle frameBindings = device.createBindingSet(
        nvrhi::BindingSetDesc().addItem(nvrhi::BindingSetItem::ConstantBuffer(0, sceneConstants)),
        frameLayout);

    nvrhi::GraphicsPipelineDesc desc;
    desc.primType = nvrhi::PrimitiveType::TriangleList;
    desc.inputLayout = inputLayout;
    desc.VS = *vertexShader;
    desc.PS = *pixelShader;
    desc.addBindingLayout(frameLayout);
    desc.renderState.depthStencilState.depthTestEnable = true;
    desc.renderState.depthStencilState.depthWriteEnable = true;
    desc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Less;
    // Faces avant dans le sens trigonométrique, comme les décrit createCube.
    desc.renderState.rasterState.frontCounterClockwise = true;
    desc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

    nvrhi::GraphicsPipelineHandle pipeline = device.createGraphicsPipeline(desc, target);
    if (!inputLayout || !frameLayout || !sceneConstants || !frameBindings || !pipeline)
    {
        return core::makeError(core::ErrorCode::InvalidData, "passe des meshes refusée par NVRHI");
    }
    return MeshPass{.vertexShader = std::move(*vertexShader),
                    .pixelShader = std::move(*pixelShader),
                    .inputLayout = std::move(inputLayout),
                    .frameLayout = std::move(frameLayout),
                    .sceneConstants = std::move(sceneConstants),
                    .frameBindings = std::move(frameBindings),
                    .pipeline = std::move(pipeline)};
}

nvrhi::ITexture* ensureDepthTexture(nvrhi::IDevice& device, nvrhi::TextureHandle& depth,
                                    std::uint32_t width, std::uint32_t height)
{
    if (depth && depth->getDesc().width == width && depth->getDesc().height == height)
    {
        return depth;
    }

    nvrhi::TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = DepthFormat;
    desc.isRenderTarget = true;
    desc.initialState = nvrhi::ResourceStates::DepthWrite;
    desc.keepInitialState = true;
    desc.debugName = "depth buffer";
    depth = device.createTexture(desc);
    return depth;
}

void drawMesh(nvrhi::ICommandList& commandList, const MeshPass& pass,
              nvrhi::IFramebuffer& framebuffer, const Mesh& mesh, const Instances& instances,
              const SceneConstants& constants)
{
    commandList.writeBuffer(pass.sceneConstants, &constants, sizeof(constants));

    nvrhi::GraphicsState state;
    state.pipeline = pass.pipeline;
    state.framebuffer = &framebuffer;
    state.viewport.addViewportAndScissorRect(framebuffer.getFramebufferInfo().getViewport());
    state.bindings = {pass.frameBindings};
    // Slot, format et décalage écrits en entier : VertexBufferBinding et IndexBufferBinding ne leur
    // donnent aucune valeur par défaut.
    state.addVertexBuffer(
        nvrhi::VertexBufferBinding().setBuffer(mesh.vertexBuffer).setSlot(0).setOffset(0));
    state.addVertexBuffer(
        nvrhi::VertexBufferBinding().setBuffer(instances.offsets).setSlot(1).setOffset(0));
    state.setIndexBuffer(nvrhi::IndexBufferBinding()
                             .setBuffer(mesh.indexBuffer)
                             .setFormat(nvrhi::Format::R32_UINT)
                             .setOffset(0));
    commandList.setGraphicsState(state);

    nvrhi::DrawArguments arguments;
    arguments.vertexCount = mesh.indexCount;
    arguments.instanceCount = instances.count;
    commandList.drawIndexed(arguments);
}

} // namespace levain::render
