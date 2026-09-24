#include "levain/assets/asset_ref.hpp"

#include <format>

namespace levain::assets
{

namespace
{

void addReference(AssetUsage& usage, const AssetRef& ref)
{
    if (ref.isSet())
    {
        ++usage.counts[ref.asset];
        usage.unused.erase(ref.asset);
    }
}

void removeReference(AssetUsage& usage, const AssetRef& ref)
{
    if (!ref.isSet())
    {
        return;
    }
    const auto count = usage.counts.find(ref.asset);
    if (count != usage.counts.end() && --count->second == 0)
    {
        usage.counts.erase(count);
        usage.unused.insert(ref.asset);
    }
}

} // namespace

AssetsModule::AssetsModule(flecs::world& world)
{
    world.module<AssetsModule>();
    world.set<AssetUsage>({});

    // Les hooks de flecs (manuel, « Component hooks ») : `on_replace` voit l'ancienne et la
    // nouvelle valeur à chaque `set`, la toute première comprise (l'ancienne est alors la valeur
    // par défaut, qui ne désigne rien) ; `on_remove` voit la valeur qui part, que le composant soit
    // retiré ou l'entité détruite. À eux deux, ils couvrent tous les chemins, testés un par un
    // (tests/asset_ref_test.cpp).
    //
    // Conséquence imposée par flecs : un `MeshRef` ne se modifie plus sur place (`get_mut`,
    // `ensure`), seulement par `set`, qui passe par `on_replace`. Et `entity.clone()` ne marche
    // plus sur une entité qui en porte un : flecs le fait par `get_mut`, et l'arrête sur une
    // assertion. On copie par `set`, et les prefabs (`IsA`) partagent le composant sans le copier.
    world.component<MeshRef>()
        .on_replace(
            [](flecs::entity entity, MeshRef& previous, MeshRef& next)
            {
                AssetUsage& usage = entity.world().get_mut<AssetUsage>();
                removeReference(usage, previous.mesh);
                addReference(usage, next.mesh);
            })
        .on_remove([](flecs::entity entity, MeshRef& ref)
                   { removeReference(entity.world().get_mut<AssetUsage>(), ref.mesh); });
}

int referenceCount(const flecs::world& world, AssetId asset)
{
    const AssetUsage& usage = world.get<AssetUsage>();
    const auto count = usage.counts.find(asset);
    return count == usage.counts.end() ? 0 : count->second;
}

std::vector<AssetId> takeUnusedAssets(flecs::world& world)
{
    AssetUsage& usage = world.get_mut<AssetUsage>();
    std::vector<AssetId> unused{usage.unused.begin(), usage.unused.end()};
    usage.unused.clear();
    return unused;
}

core::Result<const Model*> loadModel(ModelCache& cache, const AssetRegistry& registry,
                                     AssetId asset)
{
    if (const auto loaded = cache.models.find(asset); loaded != cache.models.end())
    {
        return &loaded->second;
    }
    const auto path = pathOf(registry, asset);
    if (!path)
    {
        return core::makeError(
            core::ErrorCode::FileNotFound,
            std::format("asset {} inconnu du registre : son fichier a disparu, ou il a été renommé "
                        "et modifié à la fois (ADR-0019)",
                        toString(asset)));
    }
    auto model = loadGltf(*path);
    if (!model)
    {
        return std::unexpected(model.error());
    }
    return &cache.models.emplace(asset, std::move(*model)).first->second;
}

} // namespace levain::assets
