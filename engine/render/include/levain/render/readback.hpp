#pragma once

#include <cstdint>
#include <vector>

#include <nvrhi/nvrhi.h>

#include "levain/core/error.hpp"

namespace levain::render
{

/// Une image relue du GPU : quatre octets par pixel (RGBA), lignes contiguës, couleurs telles que
/// la cible les contient (en sRGB pour la swapchain).
struct ReadbackImage
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba;
};

/// Enregistre dans `commandList` la copie de `texture` vers une texture lisible par le CPU (une
/// *staging texture* de NVRHI). La copie n'a lieu qu'à l'exécution de la command list : lire le
/// résultat avec `readBack`, après.
[[nodiscard]] nvrhi::StagingTextureHandle
copyForReadback(nvrhi::IDevice& device, nvrhi::ICommandList& commandList, nvrhi::ITexture& texture);

/// Attend que le GPU ait fini, puis lit la copie. Les formats BGRA, celui de la swapchain, sont
/// remis en RGBA. Échoue pour un format qui n'est pas de 8 bits par canal.
[[nodiscard]] core::Result<ReadbackImage> readBack(nvrhi::IDevice& device,
                                                   nvrhi::IStagingTexture& staging);

} // namespace levain::render
