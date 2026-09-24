#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "levain/core/error.hpp"

namespace levain::assets
{

/// L'identité d'un asset : 128 bits tirés au hasard, écrits dans son `.meta` (ADR-0019). Elle ne
/// dépend ni du chemin ni du contenu : renommer ou modifier le fichier la conserve.
struct AssetId
{
    std::uint64_t high = 0;
    std::uint64_t low = 0;

    friend auto operator<=>(const AssetId&, const AssetId&) = default;
};

/// Une référence à un asset, la seule chose qu'un composant ou un fichier cuit en garde
/// (ADR-0019, ADR-0020) : son GUID, et l'indice d'un sous-asset (un mesh d'un glTF, une image
/// embarquée dans un glTF). Jamais de chemin.
struct AssetRef
{
    AssetId asset;
    std::uint32_t sub = 0;

    /// Une référence par défaut ne désigne rien : un GUID nul n'est jamais tiré.
    [[nodiscard]] bool isSet() const { return asset != AssetId{}; }

    friend auto operator<=>(const AssetRef&, const AssetRef&) = default;
};

/// Un identifiant neuf. Deux tirages ne se rencontrent pas en pratique : 2^64 identifiants avant
/// une chance sur deux de collision.
[[nodiscard]] AssetId generateAssetId();

/// Les 32 chiffres hexadécimaux du `.meta`, sans tiret.
[[nodiscard]] std::string toString(AssetId id);
[[nodiscard]] std::optional<AssetId> parseAssetId(std::string_view text);

/// Le hash du contenu d'un fichier, gardé dans son `.meta` pour le reconnaître après un renommage
/// (ADR-0019). FNV-1a sur 64 bits, taille comprise : il distingue les fichiers d'un projet, il ne
/// protège pas d'un fichier malveillant.
[[nodiscard]] std::uint64_t contentHash(std::span<const std::byte> bytes);

/// Ce que dit un `.meta`.
struct AssetMeta
{
    AssetId id;
    std::uint64_t hash = 0;
};

/// Le `.meta` d'un fichier : son chemin complet suivi de « .meta ».
[[nodiscard]] std::filesystem::path metaPathOf(const std::filesystem::path& asset);

/// Lit un `.meta` (texte « clé = valeur », `#` pour les commentaires). Un `.meta` sans GUID valide
/// est un échec : un asset sans identité ne se laisse pas référencer.
[[nodiscard]] core::Result<AssetMeta> readMeta(const std::filesystem::path& path);
[[nodiscard]] core::Result<void> writeMeta(const std::filesystem::path& path,
                                           const AssetMeta& meta);

} // namespace levain::assets
