#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "levain/assets/image.hpp"
#include "levain/core/error.hpp"

namespace levain::assets
{

/// Le format d'une texture prête pour le GPU. `assets` ne connaît pas NVRHI : l'application fait
/// la correspondance.
enum class TextureFormat : std::uint8_t
{
    Rgba8Srgb, ///< 4 octets par pixel, non compressé.
    Bc7Srgb,   ///< Blocs de 4 × 4 pixels, 16 octets chacun : 4 fois moins de mémoire vidéo.
};

struct TextureMip
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::byte> bytes;
};

/// Une texture et tous ses niveaux, le premier étant le plus grand : ce que le GPU recevra.
struct TextureData
{
    TextureFormat format = TextureFormat::Rgba8Srgb;
    std::vector<TextureMip> mips;
};

/// Une chaîne de mips en RGBA, telle quelle : le chemin des sources.
[[nodiscard]] TextureData textureDataOf(const std::vector<Image>& mips);

/// Écrit `mips` en KTX2 (ADR-0020) : encodées en UASTC (Basis Universal), supercompressées en zstd,
/// avec la version du cuiseur et `sourceHash` dans les métadonnées. Sans GPU. Lent : l'encodage
/// UASTC est la partie chère de la cuisson.
[[nodiscard]] core::Result<void> writeCookedTexture(const std::filesystem::path& path,
                                                    const std::vector<Image>& mips,
                                                    std::uint64_t sourceHash);

/// Transcode une fois pour toutes le `.ktx2` UASTC `master` vers `target`, et l'écrit dans
/// `platform` : le cache d'une plateforme (ADR-0020, amendement du 24/09). Le chargement n'a plus
/// rien à transcoder : 9,4 ms de transcodage par texture de 1024 × 1024, mesurés, évités.
[[nodiscard]] core::Result<void> writePlatformTexture(const std::filesystem::path& master,
                                                      const std::filesystem::path& platform,
                                                      TextureFormat target);

/// Relit un `.ktx2` cuit et le transcode vers `target` : BC7 si le GPU le prend en charge, RGBA8
/// sinon. Échoue s'il est illisible ou **périmé** (autre cuiseur, autre source), comme un
/// `.lvmesh`.
[[nodiscard]] core::Result<TextureData> readCookedTexture(const std::filesystem::path& path,
                                                          std::uint64_t sourceHash,
                                                          TextureFormat target);

} // namespace levain::assets
