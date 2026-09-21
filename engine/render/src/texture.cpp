#include "levain/render/texture.hpp"

#include <cstddef>

#include "levain/core/assert.hpp"

namespace levain::render
{

nvrhi::TextureHandle createTexture(nvrhi::IDevice& device, nvrhi::ICommandList& commandList,
                                   std::span<const TextureLevel> levels, const char* debugName)
{
    LEVAIN_ASSERT(!levels.empty(), "une texture a au moins un niveau");

    nvrhi::TextureDesc desc;
    desc.width = levels.front().width;
    desc.height = levels.front().height;
    desc.mipLevels = static_cast<std::uint32_t>(levels.size());
    desc.format = nvrhi::Format::SRGBA8_UNORM;
    // L'état où la texture passe sa vie : lue par les shaders. NVRHI place la barrière vers
    // CopyDest pour l'envoi, puis revient à cet état tout seul.
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    desc.debugName = debugName;
    nvrhi::TextureHandle texture = device.createTexture(desc);

    // writeTexture passe par un buffer d'envoi interne à NVRHI, comme writeBuffer.
    for (std::size_t mip = 0; mip < levels.size(); ++mip)
    {
        const TextureLevel& level = levels[mip];
        commandList.writeTexture(texture, 0, static_cast<std::uint32_t>(mip), level.rgba.data(),
                                 std::size_t{level.width} * 4);
    }
    return texture;
}

} // namespace levain::render
