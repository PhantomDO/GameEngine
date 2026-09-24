#pragma once

#include <cstdint>
#include <span>

#include <nvrhi/nvrhi.h>

namespace levain::render
{

/// Un niveau de mip à envoyer, lignes contiguës, au format de la texture : 4 octets par pixel en
/// RGBA8, 16 octets par bloc de 4 × 4 pixels en BC7. `render` ne connaît pas `assets` (SPECS §7,
/// les deux modules sont sur des branches séparées) : l'appelant fait le lien, champ par champ.
struct TextureLevel
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::span<const std::byte> bytes;
};

/// Comment un matériau lit sa texture. Trilinéaire toujours : filtrage entre texels et entre
/// niveaux de mip.
struct SamplerSettings
{
    /// Filtrage anisotrope : nombre d'échantillons au plus le long de la direction où la texture
    /// est écrasée, sur une surface vue de biais. 1 le désactive ; ramené dans [1, 16].
    float maxAnisotropy = 1.0f;
    /// Au-delà de [0, 1] : Wrap répète la texture, Clamp étire son bord.
    nvrhi::SamplerAddressMode addressMode = nvrhi::SamplerAddressMode::Wrap;
};

/// Le niveau d'anisotropie ramené dans [1, 16]. 16 est le maximum de Direct3D 12, et le minimum que
/// Vulkan garantit dès que le GPU a la fonctionnalité samplerAnisotropy.
[[nodiscard]] float clampAnisotropy(float requested);

[[nodiscard]] nvrhi::SamplerHandle createSampler(nvrhi::IDevice& device,
                                                 const SamplerSettings& settings);

/// Crée une texture avec tous ses niveaux de mip, le premier de `levels` étant le plus grand, et
/// enregistre leur envoi dans `commandList`. Formats sRGB : le GPU reconvertit chaque lecture en
/// lumière linéaire, avant le filtrage. Un format compressé (BC7) reste compressé en mémoire vidéo,
/// et le GPU le décode à la lecture.
[[nodiscard]] nvrhi::TextureHandle
createTexture(nvrhi::IDevice& device, nvrhi::ICommandList& commandList,
              std::span<const TextureLevel> levels, const char* debugName,
              nvrhi::Format format = nvrhi::Format::SRGBA8_UNORM);

/// Le GPU sait-il échantillonner ce format ? Le BC7 est partout sur PC, pas sur mobile.
[[nodiscard]] bool supportsSampledFormat(nvrhi::IDevice& device, nvrhi::Format format);

} // namespace levain::render
