// Test de fumée du rendu (#16) : dessine le triangle hors écran, relit l'image et la compare à la
// référence tests/data/triangle.ppm. En CI, il tourne sur lavapipe, le Vulkan logiciel de Mesa.
//
// Mettre à jour la référence après un changement voulu du rendu :
//   LEVAIN_UPDATE_REFERENCE=1 SDL_VIDEO_DRIVER=offscreen
//   ./build/linux-debug/tests/levain_smoke_triangle

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <print>
#include <string>
#include <vector>

#include "levain/core/file.hpp"
#include "levain/gpu/device.hpp"
#include "levain/platform/window.hpp"
#include "levain/render/triangle.hpp"

namespace
{

constexpr int ImageSize = 64;

/// La cible est en RGBA8 : quatre octets par texel, dont l'image ne garde que les trois premiers.
constexpr std::size_t BytesPerTexel = 4;

/// Deux pixels « identiques » à ±2 près par canal : l'interpolation des couleurs peut varier d'une
/// unité d'un pilote à l'autre. Un triangle absent ou déplacé change des centaines de pixels.
constexpr int ChannelTolerance = 2;

struct Image
{
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgb; ///< Trois octets par pixel, ligne par ligne.
};

/// Le format PPM binaire (P6) : un en-tête texte et les pixels bruts. Pas de dépendance, et il
/// s'ouvre dans n'importe quelle visionneuse d'images.
bool writePpm(const std::filesystem::path& path, const Image& image)
{
    std::ofstream file{path, std::ios::binary};
    file << std::format("P6\n{} {}\n255\n", image.width, image.height);
    file.write(reinterpret_cast<const char*>(image.rgb.data()),
               static_cast<std::streamsize>(image.rgb.size()));
    return static_cast<bool>(file);
}

levain::core::Result<Image> readPpm(const std::filesystem::path& path)
{
    auto bytes = levain::core::readFile(path);
    if (!bytes)
    {
        return std::unexpected(bytes.error());
    }
    const std::string header = std::format("P6\n{} {}\n255\n", ImageSize, ImageSize);
    const std::size_t expectedSize = header.size() + std::size_t{ImageSize} * ImageSize * 3;
    if (bytes->size() != expectedSize ||
        std::memcmp(bytes->data(), header.data(), header.size()) != 0)
    {
        return levain::core::makeError(
            levain::core::ErrorCode::InvalidData,
            std::format("{} : pas une image PPM {}×{}", path.string(), ImageSize, ImageSize));
    }

    Image image{.width = ImageSize, .height = ImageSize, .rgb = {}};
    const auto* pixels = reinterpret_cast<const std::uint8_t*>(bytes->data()) + header.size();
    image.rgb.assign(pixels, pixels + (expectedSize - header.size()));
    return image;
}

/// Pixels dont au moins un canal s'écarte de plus de la tolérance.
int countDifferentPixels(const Image& actual, const Image& reference)
{
    int different = 0;
    for (std::size_t i = 0; i < actual.rgb.size(); i += 3)
    {
        for (std::size_t channel = 0; channel < 3; ++channel)
        {
            if (std::abs(actual.rgb[i + channel] - reference.rgb[i + channel]) > ChannelTolerance)
            {
                ++different;
                break;
            }
        }
    }
    return different;
}

/// Dessine le triangle dans une texture hors écran, puis la recopie dans une texture que le CPU
/// peut lire.
levain::core::Result<Image> renderTriangle(nvrhi::IDevice& device)
{
    nvrhi::TextureDesc targetDesc;
    targetDesc.width = ImageSize;
    targetDesc.height = ImageSize;
    targetDesc.format = nvrhi::Format::RGBA8_UNORM;
    targetDesc.isRenderTarget = true;
    targetDesc.initialState = nvrhi::ResourceStates::RenderTarget;
    targetDesc.keepInitialState = true;
    targetDesc.debugName = "cible du test de fumée";
    const nvrhi::TextureHandle target = device.createTexture(targetDesc);
    const nvrhi::FramebufferHandle framebuffer =
        device.createFramebuffer(nvrhi::FramebufferDesc().addColorAttachment(target));

    auto triangle = levain::render::createTrianglePass(device, framebuffer->getFramebufferInfo());
    if (!triangle)
    {
        return std::unexpected(triangle.error());
    }

    nvrhi::TextureDesc stagingDesc = targetDesc;
    stagingDesc.isRenderTarget = false;
    stagingDesc.initialState = nvrhi::ResourceStates::CopyDest;
    stagingDesc.debugName = "relecture du test de fumée";
    const nvrhi::StagingTextureHandle staging =
        device.createStagingTexture(stagingDesc, nvrhi::CpuAccessMode::Read);

    const nvrhi::CommandListHandle commandList = device.createCommandList();
    commandList->open();
    commandList->clearTextureFloat(target, nvrhi::AllSubresources,
                                   nvrhi::Color{0.0f, 0.0f, 0.0f, 1.0f});
    levain::render::drawTriangle(*commandList, *triangle, *framebuffer);
    commandList->copyTexture(staging, nvrhi::TextureSlice{}, target, nvrhi::TextureSlice{});
    commandList->close();
    device.executeCommandList(commandList);
    device.waitForIdle();

    std::size_t rowPitch = 0;
    const auto* mapped = static_cast<const std::uint8_t*>(device.mapStagingTexture(
        staging, nvrhi::TextureSlice{}, nvrhi::CpuAccessMode::Read, &rowPitch));
    if (mapped == nullptr)
    {
        return levain::core::makeError(levain::core::ErrorCode::Unsupported,
                                       "relecture impossible");
    }

    // La texture est en RGBA, lignes de rowPitch octets ; l'image garde RGB, lignes contiguës.
    Image image{.width = ImageSize, .height = ImageSize, .rgb = {}};
    image.rgb.reserve(std::size_t{ImageSize} * ImageSize * 3);
    for (int y = 0; y < ImageSize; ++y)
    {
        const std::uint8_t* row = mapped + static_cast<std::size_t>(y) * rowPitch;
        for (std::size_t x = 0; x < ImageSize; ++x)
        {
            const std::uint8_t* texel = row + x * BytesPerTexel;
            image.rgb.insert(image.rgb.end(), texel, texel + 3);
        }
    }
    device.unmapStagingTexture(staging);
    return image;
}

int runSmokeTest()
{
    auto window = levain::platform::createWindow("Levain - test de fumée", ImageSize, ImageSize);
    if (!window)
    {
        std::println(stderr, "{}", window.error().message);
        return 1;
    }
    auto gpu = levain::gpu::createGpuDevice(*window, {.enableValidation = true});
    if (!gpu)
    {
        std::println(stderr, "{}", gpu.error().message);
        return 1;
    }

    auto actual = renderTriangle(*gpu->nvrhi);
    if (!actual)
    {
        std::println(stderr, "{}", actual.error().message);
        return 1;
    }

    const std::filesystem::path referencePath = LEVAIN_REFERENCE_IMAGE;
    if (std::getenv("LEVAIN_UPDATE_REFERENCE") != nullptr)
    {
        std::println("référence réécrite : {}", referencePath.string());
        return writePpm(referencePath, *actual) ? 0 : 1;
    }

    auto reference = readPpm(referencePath);
    if (!reference)
    {
        std::println(stderr, "{}", reference.error().message);
        return 1;
    }

    const int different = countDifferentPixels(*actual, *reference);
    std::println("{} pixels sur {} différents de la référence (tolérance ±{})", different,
                 ImageSize * ImageSize, ChannelTolerance);
    if (different == 0)
    {
        return 0;
    }

    // L'image obtenue reste à côté du binaire, pour la comparer à l'œil à la référence.
    writePpm("triangle.actual.ppm", *actual);
    return 1;
}

} // namespace

int main()
{
    try
    {
        return runSmokeTest();
    }
    catch (const std::exception& e)
    {
        std::fputs(e.what(), stderr);
        std::fputc('\n', stderr);
        return 1;
    }
}
