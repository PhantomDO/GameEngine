// levain_cook : cuit les assets d'une ou plusieurs racines (ADR-0020). Sans GPU : il tourne sur une
// machine de build, comme le calcul des mips (M2.2).
//
//   levain_cook data assets-cache
//
// Chaque glTF devient <racine>/.cooked/<guid>.lvmesh ; chaque image, et chaque image embarquée
// dans un glTF, un maître UASTC (<guid>.ktx2) et le cache BC7 du PC (<guid>.bc7.ktx2). Un fichier
// déjà à jour (même hash de source, même version du cuiseur) n'est pas refait.

#include <chrono>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <span>
#include <string_view>

#include "levain/assets/asset_ref.hpp"
#include "levain/assets/cooked.hpp"
#include "levain/assets/cooked_texture.hpp"
#include "levain/assets/gltf.hpp"
#include "levain/assets/image.hpp"
#include "levain/assets/registry.hpp"
#include "levain/core/log.hpp"

namespace
{

namespace fs = std::filesystem;
using levain::core::log;
using levain::core::LogLevel;

bool isModel(const fs::path& path)
{
    const fs::path extension = path.extension();
    return extension == ".gltf" || extension == ".glb";
}

fs::path withSuffix(fs::path stem, std::string_view suffix)
{
    stem += suffix;
    return stem;
}

/// Cuit une texture (ADR-0020, et son amendement du 24/09) : le maître UASTC, puis le cache BC7 du
/// PC transcodé depuis lui. Rien n'est refait si le cache est à jour. Rend faux en cas d'échec.
bool cookTexture(const fs::path& stem, const levain::assets::Image& image, std::uint64_t hash)
{
    const fs::path master = withSuffix(stem, ".ktx2");
    const fs::path platform = withSuffix(stem, ".bc7.ktx2");
    if (levain::assets::readCookedTexture(platform, hash, levain::assets::TextureFormat::Bc7Srgb))
    {
        return true;
    }
    const auto start = std::chrono::steady_clock::now();
    auto written =
        levain::assets::writeCookedTexture(master, levain::assets::buildMipChain(image), hash);
    if (written)
    {
        written = levain::assets::writePlatformTexture(master, platform,
                                                       levain::assets::TextureFormat::Bc7Srgb);
    }
    if (!written)
    {
        log("cook", LogLevel::Error, "{}", written.error().message);
        return false;
    }
    log("cook", LogLevel::Info, "texture cuite en {:.0f} ms : {}",
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count(),
        master.filename().string());
    return true;
}

/// Cuit un modèle, s'il n'est pas déjà à jour. Rend faux en cas d'échec.
bool cookModel(const levain::assets::AssetRegistry& registry, levain::assets::AssetId id,
               const levain::assets::AssetEntry& entry)
{
    const fs::path cooked =
        levain::assets::cookedPathOf(registry, id, ".lvmesh").value_or(fs::path{});
    if (levain::assets::readCookedModel(cooked, entry.hash))
    {
        log("cook", LogLevel::Info, "à jour : {}", entry.file.string());
        return true;
    }
    const auto start = std::chrono::steady_clock::now();
    auto model = levain::assets::loadGltf(entry.file, id, registry);
    // Ses images embarquées sont des textures comme les autres, désignées par {GUID, indice}.
    if (model)
    {
        for (const auto& [index, image] : model->embeddedImages)
        {
            const auto stem =
                levain::assets::cookedTextureStem(registry, {.asset = id, .sub = index});
            if (!stem || !cookTexture(*stem, image, entry.hash))
            {
                return false;
            }
        }
    }
    auto written = model ? levain::assets::writeCookedModel(cooked, *model, entry.hash)
                         : std::unexpected(model.error());
    if (!written)
    {
        log("cook", LogLevel::Error, "{}", written.error().message);
        return false;
    }
    log("cook", LogLevel::Info, "cuit en {:.0f} ms : {} → {}",
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count(),
        entry.file.string(), cooked.string());
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        const std::span arguments{argv, static_cast<std::size_t>(argc)};
        if (arguments.size() < 2)
        {
            std::fputs("usage : levain_cook <racine d'assets>...\n", stderr);
            return 2;
        }

        // Toutes les racines d'abord : un glTF peut désigner une image d'une autre racine.
        levain::assets::AssetRegistry registry;
        for (const char* root : arguments.subspan(1))
        {
            if (auto report = levain::assets::scanAssets(root, registry); !report)
            {
                log("cook", LogLevel::Error, "{}", report.error().message);
                return 1;
            }
        }

        int failures = 0;
        int models = 0;
        int textures = 0;
        for (const auto& [id, entry] : registry.entries)
        {
            if (isModel(entry.file))
            {
                ++models;
                failures += cookModel(registry, id, entry) ? 0 : 1;
                continue;
            }
            ++textures;
            auto image = levain::assets::loadImage(entry.file);
            const auto stem = levain::assets::cookedTextureStem(registry, {.asset = id, .sub = 0});
            const bool cooked = image && stem && cookTexture(*stem, *image, entry.hash);
            if (!image)
            {
                log("cook", LogLevel::Error, "{}", image.error().message);
            }
            failures += cooked ? 0 : 1;
        }
        // Rien à cuire est suspect (règle n°7) : une racine mal orthographiée ne doit pas passer
        // pour un succès.
        if (models + textures == 0)
        {
            log("cook", LogLevel::Error, "aucun asset dans les racines données");
            return 1;
        }
        log("cook", LogLevel::Info, "{} modèles, {} images, {} échecs", models, textures, failures);
        return failures == 0 ? 0 : 1;
    }
    catch (const std::exception& e)
    {
        std::fputs(e.what(), stderr);
        std::fputc('\n', stderr);
        return 1;
    }
}
