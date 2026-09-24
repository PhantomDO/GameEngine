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
                                   std::span<const TextureLevel> levels, const char* debugName,
                                   nvrhi::Format format)
{
    LEVAIN_ASSERT(!levels.empty(), "une texture a au moins un niveau");

    nvrhi::TextureDesc desc;
    desc.width = levels.front().width;
    desc.height = levels.front().height;
    desc.mipLevels = static_cast<std::uint32_t>(levels.size());
    desc.format = format;
    // L'état où la texture passe sa vie : lue par les shaders. NVRHI place la barrière vers
    // CopyDest pour l'envoi, puis revient à cet état tout seul.
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    desc.debugName = debugName;
    nvrhi::TextureHandle texture = device.createTexture(desc);

    // writeTexture passe par un buffer d'envoi interne à NVRHI, comme writeBuffer. Le pas d'une
    // ligne se compte en blocs : 1 × 1 pixel pour le RGBA8, 4 × 4 pour le BC7 (qui arrondit au
    // bloc supérieur les niveaux de moins de 4 pixels).
    const nvrhi::FormatInfo& info = nvrhi::getFormatInfo(format);
    for (std::size_t mip = 0; mip < levels.size(); ++mip)
    {
        const TextureLevel& level = levels[mip];
        const std::size_t blocksPerRow = (level.width + info.blockSize - 1) / info.blockSize;
        commandList.writeTexture(texture, 0, static_cast<std::uint32_t>(mip), level.bytes.data(),
                                 blocksPerRow * info.bytesPerBlock);
    }
    return texture;
}

bool supportsSampledFormat(nvrhi::IDevice& device, nvrhi::Format format)
{
    const nvrhi::FormatSupport support = device.queryFormatSupport(format);
    return (support & nvrhi::FormatSupport::Texture) == nvrhi::FormatSupport::Texture &&
           (support & nvrhi::FormatSupport::ShaderSample) == nvrhi::FormatSupport::ShaderSample;
}

} // namespace levain::render
