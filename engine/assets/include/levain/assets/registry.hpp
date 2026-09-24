#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <vector>

#include "levain/assets/asset_id.hpp"
#include "levain/core/error.hpp"

namespace levain::assets
{

/// Où se trouve chaque asset connu. C'est le seul endroit du moteur où vivent des chemins : les
/// composants et les scènes ne gardent que des `AssetId` (ADR-0019).
struct AssetRegistry
{
    std::map<AssetId, std::filesystem::path> paths;
};

/// Ce que le scan a changé sur le disque, pour le journal : ces `.meta` sont à versionner.
struct ScanReport
{
    std::vector<std::filesystem::path> created;    ///< Nouveaux assets, `.meta` écrit.
    std::vector<std::filesystem::path> reattached; ///< Renommés hors du moteur, GUID conservé.
    std::vector<std::filesystem::path> orphans;    ///< `.meta` sans fichier : asset disparu.
};

/// Les fichiers que le moteur sait importer : images et modèles glTF. Les `.bin` d'un glTF n'en
/// font pas partie : ils appartiennent à leur `.gltf`.
[[nodiscard]] bool isImportable(const std::filesystem::path& path);

/// Parcourt `root` et ses sous-dossiers, et enregistre ses assets dans `registry` (ADR-0019) :
///   1. un fichier et son `.meta` : enregistré ; le hash est mis à jour s'il a été modifié ;
///   2. un fichier sans `.meta`, et un `.meta` orphelin de même hash : un renommage, le `.meta`
///   suit ;
///   3. un fichier sans `.meta` ni orphelin qui corresponde : un nouveau GUID, un `.meta` écrit ;
///   4. un `.meta` resté orphelin : signalé dans le rapport ;
///   5. deux fichiers de même GUID : échec, qui nomme les deux, sans choisir à la place de
///   personne.
[[nodiscard]] core::Result<ScanReport> scanAssets(const std::filesystem::path& root,
                                                  AssetRegistry& registry);

/// Le GUID du fichier `path`, s'il est enregistré. Deux écritures du même fichier (relative et
/// absolue) le trouvent toutes les deux. Linéaire : pour les outils et le démarrage, pas par image.
[[nodiscard]] std::optional<AssetId> idOf(const AssetRegistry& registry,
                                          const std::filesystem::path& path);

/// Le chemin d'un asset, s'il est connu.
[[nodiscard]] std::optional<std::filesystem::path> pathOf(const AssetRegistry& registry,
                                                          AssetId id);

} // namespace levain::assets
