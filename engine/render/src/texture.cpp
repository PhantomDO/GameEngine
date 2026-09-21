#include "levain/render/texture.hpp"

#include <algorithm>
#include <cstddef>

#include "levain/core/assert.hpp"

namespace levain::render
{

float clampAnisotropy(float requested)
{
    // !(x >= 1) et non x < 1 : un NaN tombe aussi à 1, au lieu de passer tel quel au pilote.
    if (!(requested >= 1.0f))
    {
        return 1.0f;
    }
    return std::min(requested, 16.0f);
}

nvrhi::SamplerHandle createSampler(nvrhi::IDevice& device, const SamplerSettings& settings)
{
    return device.createSampler(nvrhi::SamplerDesc()
                                    .setAllFilters(true)
                                    .setMaxAnisotropy(clampAnisotropy(settings.maxAnisotropy))
                                    .setAllAddressModes(settings.addressMode));
}

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
