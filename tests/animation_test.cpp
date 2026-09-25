#include <cmath>
#include <string>

#include <doctest/doctest.h>
#include <glm/glm.hpp>

#include "levain/animation/animation_set.hpp"
#include "levain/animation/pose.hpp"

// tests/data/two-joints.gltf : deux os, « racine » puis « bout » un mètre au-dessus, sous un nœud
// « porteur » qui n'est pas un os. Deux clips : « tourne », la racine tourne de 0 à 90° autour de
// Z en une seconde (LINEAR) ; « saute », la racine passe de x = 0 à x = 2 à la première seconde
// (STEP), sur deux secondes.

namespace
{

levain::animation::AnimationSet twoJoints()
{
    auto set = levain::animation::importAnimationSet(LEVAIN_TEST_DATA_DIR "/two-joints.gltf");
    REQUIRE(set.has_value());
    return std::move(*set);
}

/// La position de l'os `joint` dans le repère du squelette.
glm::vec3 positionOf(const levain::animation::Pose& pose, std::size_t joint)
{
    return glm::vec3(pose.joints[joint][3]);
}

} // namespace

TEST_CASE("la passerelle garde les os du skin, parent d'abord, et les clips du glTF")
{
    const levain::animation::AnimationSet set = twoJoints();

    REQUIRE(set.jointNames.size() == 2); // « porteur » n'est pas un os
    CHECK(set.jointNames[0] == "racine");
    CHECK(set.jointNames[1] == "bout");
    REQUIRE(set.clips.size() == 2);
    CHECK(set.clips[0].name == "tourne");
    CHECK(set.clips[0].durationSeconds == doctest::Approx(1.0f));
    CHECK(set.clips[1].name == "saute");
    CHECK(set.clips[1].durationSeconds == doctest::Approx(2.0f));
}

TEST_CASE("un clip LINEAR s'interpole, et la hiérarchie compose les os")
{
    const levain::animation::AnimationSet set = twoJoints();
    levain::animation::Pose pose;

    // À mi-clip, la racine a tourné de 45° : le bout, un mètre au-dessus d'elle, penche vers -x.
    levain::animation::samplePose(set, 0, 0.5f, pose);
    REQUIRE(pose.joints.size() == 2);
    const float half = std::sqrt(0.5f);
    CHECK(positionOf(pose, 1).x == doctest::Approx(-half).epsilon(1e-3));
    CHECK(positionOf(pose, 1).y == doctest::Approx(half).epsilon(1e-3));
    // Le porteur n'entre pas dans la pose : elle est dans le repère du squelette.
    CHECK(positionOf(pose, 0).z == doctest::Approx(0.0f));
}

TEST_CASE("un clip STEP tient sa valeur jusqu'à la clé suivante, et un clip boucle")
{
    const levain::animation::AnimationSet set = twoJoints();
    levain::animation::Pose pose;

    levain::animation::samplePose(set, 1, 0.9f, pose);
    CHECK(positionOf(pose, 0).x == doctest::Approx(0.0f));
    levain::animation::samplePose(set, 1, 1.5f, pose);
    CHECK(positionOf(pose, 0).x == doctest::Approx(2.0f));
    // 2,9 s dans un clip de 2 s : 0,9 s, avant la marche.
    levain::animation::samplePose(set, 1, 2.9f, pose);
    CHECK(positionOf(pose, 0).x == doctest::Approx(0.0f));
}

TEST_CASE("un glTF sans skin est un échec récupérable")
{
    const auto set = levain::animation::importAnimationSet(LEVAIN_TEST_DATA_DIR "/two-nodes.gltf");
    REQUIRE_FALSE(set.has_value());
    CHECK(set.error().message.find("aucun skin") != std::string::npos);
}
