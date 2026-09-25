#include <array>
#include <cmath>
#include <string>
#include <vector>

#include <doctest/doctest.h>
#include <glm/glm.hpp>

#include "levain/animation/animation_set.hpp"
#include "levain/animation/locomotion.hpp"
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

TEST_CASE("le skin suit l'ordre du glTF, et ses matrices valent l'identité au repos")
{
    const levain::animation::AnimationSet set = twoJoints();
    levain::animation::Pose pose;
    std::vector<glm::mat4> matrices;

    // Le skin nomme « bout » puis « racine », l'inverse de l'ordre de la pose.
    REQUIRE(set.skinJoints.size() == 2);
    CHECK(set.skinJoints[0] == 1);
    CHECK(set.skinJoints[1] == 0);

    // Au début de « tourne », le squelette est dans sa pose de liaison. À 1e-3 près : ozz stocke
    // ses clés de rotation sur 15 bits par composante (animation_keyframe.h), d'où un écart de
    // l'ordre de 1e-4.
    levain::animation::samplePose(set, 0, 0.0f, pose);
    levain::animation::skinningMatrices(set, pose, matrices);
    for (const glm::mat4& matrix : matrices)
    {
        for (int column = 0; column < 4; ++column)
        {
            for (int row = 0; row < 4; ++row)
            {
                CHECK(matrix[column][row] ==
                      doctest::Approx(column == row ? 1.0f : 0.0f).epsilon(1e-3));
            }
        }
    }
}

TEST_CASE("un sommet lié au bout suit sa rotation, dans le repère du modèle")
{
    const levain::animation::AnimationSet set = twoJoints();
    levain::animation::Pose pose;
    std::vector<glm::mat4> matrices;

    // Au repos, le bout est en (0, 1, 5) : un mètre au-dessus de la racine, que le porteur place en
    // z = 5. À mi-clip, la racine a tourné de 45° autour de Z.
    levain::animation::samplePose(set, 0, 0.5f, pose);
    levain::animation::skinningMatrices(set, pose, matrices);
    const glm::vec3 moved = matrices[0] * glm::vec4{0.0f, 1.0f, 5.0f, 1.0f};
    const float half = std::sqrt(0.5f);
    CHECK(moved.x == doctest::Approx(-half).epsilon(1e-3));
    CHECK(moved.y == doctest::Approx(half).epsilon(1e-3));
    CHECK(moved.z == doctest::Approx(5.0f).epsilon(1e-3));
}

TEST_CASE("deux poses mélangées à parts égales donnent la pose du milieu")
{
    const levain::animation::AnimationSet set = twoJoints();
    levain::animation::Pose pose;

    // « tourne » au début (0°) et à la fin (90°) : le mélange est à 45°.
    const std::array<levain::animation::ClipLayer, 2> layers{
        levain::animation::ClipLayer{.clip = 0, .ratio = 0.0f, .weight = 0.5f},
        levain::animation::ClipLayer{.clip = 0, .ratio = 1.0f, .weight = 0.5f}};
    levain::animation::sampleBlend(set, layers, pose);
    const float half = std::sqrt(0.5f);
    CHECK(positionOf(pose, 1).x == doctest::Approx(-half).epsilon(1e-3));
    CHECK(positionOf(pose, 1).y == doctest::Approx(half).epsilon(1e-3));
}

TEST_CASE("la vitesse répartit les poids entre repos, marche et course")
{
    const levain::animation::Locomotion locomotion{
        .idle = 0, .walk = 1, .run = 2, .walkSpeed = 1.0f, .runSpeed = 3.0f};
    using Weights = std::array<float, 3>;

    CHECK(levain::animation::strideWeightsOf(locomotion, 0.0f) == Weights{1.0f, 0.0f, 0.0f});
    CHECK(levain::animation::strideWeightsOf(locomotion, 0.5f) == Weights{0.5f, 0.5f, 0.0f});
    CHECK(levain::animation::strideWeightsOf(locomotion, 2.0f) == Weights{0.0f, 0.5f, 0.5f});
    CHECK(levain::animation::strideWeightsOf(locomotion, 9.0f) == Weights{0.0f, 0.0f, 1.0f});
}

TEST_CASE("la foulée dure la moyenne pondérée de la marche et de la course, et s'arrête avec elles")
{
    const levain::animation::AnimationSet set = twoJoints();
    // La marche est « tourne » (1 s), la course « saute » (2 s).
    const levain::animation::Locomotion locomotion{
        .idle = 1, .walk = 0, .run = 1, .walkSpeed = 1.0f, .runSpeed = 3.0f};
    levain::animation::LocomotionClock clock;

    // À mi-chemin entre marche et course, une foulée dure 1,5 s : 0,3 s en font le cinquième.
    auto layers = levain::animation::advanceLocomotion(set, locomotion, clock, 2.0f, 0.3f);
    CHECK(clock.stridePhase == doctest::Approx(0.2f));
    CHECK(layers[1].ratio == layers[2].ratio); // marche et course au même pas
    CHECK(layers[0].weight == doctest::Approx(0.0f));

    // À l'arrêt, la phase attend ; seul le repos avance.
    layers = levain::animation::advanceLocomotion(set, locomotion, clock, 0.0f, 0.5f);
    CHECK(clock.stridePhase == doctest::Approx(0.2f));
    CHECK(layers[0].weight == doctest::Approx(1.0f));
    CHECK(clock.idleSeconds == doctest::Approx(0.8f));
}
