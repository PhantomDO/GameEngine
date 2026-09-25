#include <string>

#include <doctest/doctest.h>

#include "levain/animation/animation_set.hpp"

// tests/data/two-joints.gltf : deux os, « racine » puis « bout » un mètre au-dessus, sous un nœud
// « porteur » qui n'est pas un os. Deux clips, que lira l'échantillonnage.

TEST_CASE("la passerelle garde les os du skin, parent d'abord")
{
    const auto set = levain::animation::importAnimationSet(LEVAIN_TEST_DATA_DIR "/two-joints.gltf");

    REQUIRE(set.has_value());
    REQUIRE(set->jointNames.size() == 2); // « porteur » n'est pas un os
    CHECK(set->jointNames[0] == "racine");
    CHECK(set->jointNames[1] == "bout");
}

TEST_CASE("un glTF sans skin est un échec récupérable")
{
    const auto set = levain::animation::importAnimationSet(LEVAIN_TEST_DATA_DIR "/two-nodes.gltf");
    REQUIRE_FALSE(set.has_value());
    CHECK(set.error().message.find("aucun skin") != std::string::npos);
}
