#include <filesystem>

#include <doctest/doctest.h>
#include <flecs.h>

#include "levain/assets/gltf.hpp"
#include "levain/scene/components.hpp"
#include "levain/scene/fixed_step.hpp"
#include "levain/scene/scene.hpp"
#include "levain/scene/transform.hpp"

using levain::assets::loadGltf;
using levain::assets::MeshInstance;
using levain::core::ErrorCode;

namespace
{

/// Un triangle, et deux nœuds qui le portent : « parent », décalé de 1 en x, et son « enfant »,
/// donné par une **matrice** (échelle 2, 2 plus haut) pour vérifier la décomposition en TRS. Une
/// fonction et non une constante : construire un chemin peut lever, et une globale ne le rattrape
/// pas.
std::filesystem::path twoNodes()
{
    return std::filesystem::path{LEVAIN_TEST_DATA_DIR} / "two-nodes.gltf";
}

} // namespace

TEST_CASE("loadGltf lit les meshes et les nœuds, parent avant enfant")
{
    const auto model = loadGltf(twoNodes());
    INFO("message d'erreur : " << (model ? std::string{} : model.error().message));
    REQUIRE(model.has_value());

    REQUIRE(model->meshes.size() == 1);
    REQUIRE(model->meshes[0].primitives.size() == 1);
    const auto& triangle = model->meshes[0].primitives[0];
    CHECK(triangle.vertices.size() == 3);
    CHECK(triangle.indices == std::vector<std::uint32_t>{0, 1, 2});
    CHECK(triangle.vertices[1].position.x == doctest::Approx(1.0f));
    CHECK(triangle.vertices[0].normal.y == doctest::Approx(1.0f)); // absente du fichier

    REQUIRE(model->nodes.size() == 2);
    CHECK(model->nodes[0].name == "parent");
    CHECK_FALSE(model->nodes[0].parent.has_value());
    CHECK(model->nodes[1].parent == 0u);
    CHECK(model->nodes[1].local.position.y == doctest::Approx(2.0f));
    CHECK(model->nodes[1].local.scale.x == doctest::Approx(2.0f));
}

TEST_CASE("loadGltf lit la couleur de base des matériaux, texture embarquée comprise")
{
    const auto model = loadGltf(twoNodes());
    REQUIRE(model.has_value());

    CHECK(model->meshes[0].primitives[0].material == 0u);
    REQUIRE(model->materials.size() == 1);
    CHECK(model->materials[0].baseColorFactor.g == doctest::Approx(0.5f));
    REQUIRE(model->materials[0].baseColorImage == 0u);
    // Un PNG de 2 × 1 en base64 dans le fichier : un pixel rouge, un pixel bleu.
    REQUIRE(model->images.size() == 1);
    CHECK(model->images[0].width == 2);
    CHECK(model->images[0].rgba == std::vector<std::uint8_t>{255, 0, 0, 255, 0, 0, 255, 255});
}

TEST_CASE("un modèle instancié garde sa hiérarchie : l'enfant suit son parent")
{
    const auto model = loadGltf(twoNodes());
    REQUIRE(model.has_value());
    flecs::world world;
    world.import<levain::scene::SceneModule>();

    const flecs::entity root = levain::assets::instantiateModel(world, *model, "modele");
    root.set(levain::scene::Transform{.position = {0.0f, 0.0f, 10.0f}});
    levain::scene::FixedStep step;
    levain::scene::advanceWorld(world, step, 0.0f);

    int meshes = 0;
    glm::vec3 childPosition{0.0f};
    world.each(
        [&](const MeshInstance&, const levain::scene::WorldTransform& world,
            const levain::scene::Transform& local)
        {
            ++meshes;
            if (local.scale.x > 1.5f) // l'enfant
            {
                childPosition = levain::scene::worldPosition(world);
            }
        });
    CHECK(meshes == 2);
    // Racine (0, 0, 10) + parent (1, 0, 0) + enfant (0, 2, 0).
    CHECK(childPosition.x == doctest::Approx(1.0f));
    CHECK(childPosition.y == doctest::Approx(2.0f));
    CHECK(childPosition.z == doctest::Approx(10.0f));
}

TEST_CASE("loadGltf signale un fichier absent, et refuse autre chose que des triangles")
{
    const auto missing = loadGltf("introuvable.gltf");
    REQUIRE_FALSE(missing.has_value());
    CHECK(missing.error().code == ErrorCode::FileNotFound);

    const auto lines = loadGltf(std::filesystem::path{LEVAIN_TEST_DATA_DIR} / "lines.gltf");
    REQUIRE_FALSE(lines.has_value());
    CHECK(lines.error().code == ErrorCode::InvalidData);
}
