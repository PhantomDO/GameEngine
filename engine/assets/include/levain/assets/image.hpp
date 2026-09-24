#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

#include "levain/core/error.hpp"

namespace levain::assets
{

/// Une image décodée, prête à envoyer au GPU : quatre octets par pixel (RGBA), lignes contiguës,
/// couleurs en sRGB comme dans le fichier.
struct Image
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba;
};

/// Charge une image PNG, JPEG, TGA ou BMP (stb_image), convertie en RGBA quel que soit son format
/// d'origine. Un fichier absent ou illisible est un échec récupérable (ADR-0008).
[[nodiscard]] core::Result<Image> loadImage(const std::filesystem::path& path);

/// Écrit une image RGBA en PNG (stb_image_write). Sert aux captures d'écran du moteur, pour
/// montrer un rendu à qui n'a pas l'écran sous les yeux.
[[nodiscard]] core::Result<void> savePng(const std::filesystem::path& path, std::uint32_t width,
                                         std::uint32_t height, std::span<const std::uint8_t> rgba);

/// Nombre de niveaux de mip, de l'image elle-même jusqu'à 1 × 1, chaque niveau divisant la taille
/// par deux : 9 pour 256 × 256, comme pour 256 × 64.
[[nodiscard]] std::uint32_t mipCountFor(std::uint32_t width, std::uint32_t height);

/// La chaîne de mipmaps de `base` : le niveau 0 est `base`, le dernier fait 1 × 1. Calculée sur le
/// CPU, parce qu'elle le sera un jour à la cuisson des assets, sans GPU.
[[nodiscard]] std::vector<Image> buildMipChain(Image base);

} // namespace levain::assets
