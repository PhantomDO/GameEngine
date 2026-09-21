#pragma once

#include <cstdint>
#include <span>

#include <nvrhi/nvrhi.h>

namespace levain::render
{

/// Un niveau de mip à envoyer : RGBA8 en sRGB, lignes contiguës. `render` ne connaît pas
/// `assets::Image` (SPECS §7, les deux modules sont sur des branches séparées) : l'appelant fait le
/// lien, champ par champ.
struct TextureLevel
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::span<const std::uint8_t> rgba;
};

/// Crée une texture avec tous ses niveaux de mip, le premier de `levels` étant le plus grand, et
/// enregistre leur envoi dans `commandList`. Format sRGB : le GPU reconvertit chaque lecture en
/// lumière linéaire, avant le filtrage.
[[nodiscard]] nvrhi::TextureHandle createTexture(nvrhi::IDevice& device,
                                                 nvrhi::ICommandList& commandList,
                                                 std::span<const TextureLevel> levels,
                                                 const char* debugName);

} // namespace levain::render
