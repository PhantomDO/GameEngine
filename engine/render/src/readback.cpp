#include "levain/render/readback.hpp"

#include <algorithm>
#include <optional>
#include <utility>

namespace levain::render
{

namespace
{

constexpr std::size_t BytesPerTexel = 4;

/// Le rouge et le bleu sont-ils échangés ? `nullopt` pour un format que la relecture ne sait pas
/// lire.
std::optional<bool> isBgra(nvrhi::Format format)
{
    switch (format)
    {
    case nvrhi::Format::RGBA8_UNORM:
    case nvrhi::Format::SRGBA8_UNORM:
        return false;
    case nvrhi::Format::BGRA8_UNORM:
    case nvrhi::Format::SBGRA8_UNORM:
        return true;
    default:
        return std::nullopt;
    }
}

} // namespace

nvrhi::StagingTextureHandle
copyForReadback(nvrhi::IDevice& device, nvrhi::ICommandList& commandList, nvrhi::ITexture& texture)
{
    nvrhi::TextureDesc desc = texture.getDesc();
    desc.isRenderTarget = false;
    desc.initialState = nvrhi::ResourceStates::CopyDest;
    desc.keepInitialState = true;
    desc.debugName = "relecture";
    nvrhi::StagingTextureHandle staging =
        device.createStagingTexture(desc, nvrhi::CpuAccessMode::Read);
    commandList.copyTexture(staging, nvrhi::TextureSlice{}, &texture, nvrhi::TextureSlice{});
    return staging;
}

core::Result<ReadbackImage> readBack(nvrhi::IDevice& device, nvrhi::IStagingTexture& staging)
{
    const nvrhi::TextureDesc& desc = staging.getDesc();
    const std::optional<bool> swapRedBlue = isBgra(desc.format);
    if (!swapRedBlue)
    {
        return core::makeError(core::ErrorCode::Unsupported,
                               "relecture : format de texture non pris en charge");
    }

    device.waitForIdle();
    std::size_t rowPitch = 0;
    const auto* mapped = static_cast<const std::uint8_t*>(device.mapStagingTexture(
        &staging, nvrhi::TextureSlice{}, nvrhi::CpuAccessMode::Read, &rowPitch));
    if (mapped == nullptr)
    {
        return core::makeError(core::ErrorCode::Unsupported, "relecture impossible");
    }

    // Les lignes du GPU font rowPitch octets, alignement compris ; celles de l'image sont
    // contiguës.
    ReadbackImage image{.width = desc.width, .height = desc.height, .rgba = {}};
    const std::size_t rowBytes = std::size_t{desc.width} * BytesPerTexel;
    image.rgba.resize(rowBytes * desc.height);
    for (std::size_t y = 0; y < desc.height; ++y)
    {
        const std::uint8_t* source = mapped + y * rowPitch;
        std::uint8_t* destination = image.rgba.data() + y * rowBytes;
        std::copy(source, source + rowBytes, destination);
        if (*swapRedBlue)
        {
            for (std::size_t x = 0; x < rowBytes; x += BytesPerTexel)
            {
                std::swap(destination[x], destination[x + 2]);
            }
        }
    }
    device.unmapStagingTexture(&staging);
    return image;
}

} // namespace levain::render
