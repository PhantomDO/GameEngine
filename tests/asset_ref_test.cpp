#include <filesystem>

#include <doctest/doctest.h>
#include <flecs.h>

#include "levain/assets/asset_ref.hpp"
#include "levain/assets/gltf.hpp"
#include "levain/assets/registry.hpp"
#include "levain/scene/scene.hpp"

using levain::assets::AssetId;
using levain::assets::AssetsModule;
using levain::assets::MeshRef;
using levain::assets::referenceCount;
using levain::assets::takeUnusedAssets;

namespace fs = std::filesystem;

namespace
{

flecs::world assetWorld()
{
    flecs::world world;
    world.import<levain::scene::SceneModule>();
    world.import<AssetsModule>();
    return world;
}

MeshRef meshOf(AssetId asset)
{
    return MeshRef{.mesh = {.asset = asset, .sub = 0}};
}

} // namespace

// Chaque chemin par lequel une référence apparaît ou disparaît, un par un : c'est la garantie que
// les hooks du module n'en oublient aucun (ADR-0019).
TEST_CASE("le monde compte les références : pose, remplacement, retrait, destruction, copie")
{
    flecs::world world = assetWorld();
    const AssetId a = levain::assets::generateAssetId();
    const AssetId b = levain::assets::generateAssetId();

    const flecs::entity first = world.entity().set(meshOf(a));
    const flecs::entity second = world.entity().set(meshOf(a));
    CHECK(referenceCount(world, a) == 2);

    second.set(meshOf(b)); // remplacement : a perd une référence, b en gagne une
    CHECK(referenceCount(world, a) == 1);
    CHECK(referenceCount(world, b) == 1);

    second.set(meshOf(b)); // la même valeur : rien ne change
    CHECK(referenceCount(world, b) == 1);

    // Une copie se fait par `set` : `clone()` passe par `get_mut`, que flecs interdit sur un
    // composant qui a un hook on_replace (assertion de flecs, et non bug du comptage).
    // La valeur est copiée d'abord : `set(first.get<MeshRef>())` passerait une référence vers la
    // table de `first`, que l'ajout à une nouvelle entité de la même table peut réallouer (ASan).
    const MeshRef value = first.get<MeshRef>();
    const flecs::entity copy = world.entity().set(value);
    CHECK(referenceCount(world, a) == 2);

    first.remove<MeshRef>();
    CHECK(referenceCount(world, a) == 1);
    copy.destruct();
    second.destruct();
    CHECK(referenceCount(world, a) == 0);
    CHECK(referenceCount(world, b) == 0);
}

TEST_CASE("un asset tombé à zéro est à décharger, une seule fois, sauf s'il revient")
{
    flecs::world world = assetWorld();
    const AssetId a = levain::assets::generateAssetId();

    const flecs::entity entity = world.entity().set(meshOf(a));
    CHECK(takeUnusedAssets(world).empty());

    entity.destruct();
    const auto unused = takeUnusedAssets(world);
    REQUIRE(unused.size() == 1);
    CHECK(unused[0] == a);
    CHECK(takeUnusedAssets(world).empty()); // déjà rendu

    // Retiré puis repris dans la même image : il ne se décharge pas.
    const flecs::entity again = world.entity().set(meshOf(a));
    again.remove<MeshRef>();
    world.entity().set(meshOf(a));
    CHECK(takeUnusedAssets(world).empty());
}

TEST_CASE("un modèle se charge par son GUID, et ses entités le référencent")
{
    const fs::path root =
        fs::temp_directory_path() /
        ("levain-asset-ref-" + levain::assets::toString(levain::assets::generateAssetId()));
    fs::create_directories(root);
    fs::copy_file(fs::path{LEVAIN_TEST_DATA_DIR} / "two-nodes.gltf", root / "two-nodes.gltf");

    levain::assets::AssetRegistry registry;
    REQUIRE(levain::assets::scanAssets(root, registry).has_value());
    REQUIRE(registry.entries.size() == 1);
    const AssetId asset = registry.entries.begin()->first;

    levain::assets::ModelCache cache;
    const auto model = levain::assets::loadModel(cache, registry, asset);
    REQUIRE(model.has_value());
    CHECK(levain::assets::loadModel(cache, registry, asset).value_or(nullptr) == *model);

    flecs::world world = assetWorld();
    levain::assets::instantiateModel(world, **model, asset, "modele");
    CHECK(referenceCount(world, asset) == 2); // deux nœuds portent le triangle

    // Un GUID inconnu : l'asset a disparu, c'est un échec et pas un modèle vide.
    CHECK_FALSE(
        levain::assets::loadModel(cache, registry, levain::assets::generateAssetId()).has_value());
    fs::remove_all(root);
}
