#include <ostream>

#include <doctest/doctest.h>
#include <glm/glm.hpp>

#include "levain/render/camera.hpp"

using levain::render::Camera;
using levain::render::viewProjectionOf;

namespace
{

/// Position dans l'espace normalisé de l'écran (NDC) : x et y de −1 à 1, profondeur z de 0 à 1.
glm::vec3 toNormalizedDevice(const glm::mat4& viewProjection, const glm::vec3& point)
{
    const glm::vec4 clip = viewProjection * glm::vec4{point, 1.0f};
    return glm::vec3{clip} / clip.w;
}

} // namespace

TEST_CASE("viewProjectionOf place la cible au centre de l'écran")
{
    const Camera camera;
    const glm::vec3 target =
        toNormalizedDevice(viewProjectionOf(camera, 16.0f / 9.0f), camera.target);

    CHECK(target.x == doctest::Approx(0.0f));
    CHECK(target.y == doctest::Approx(0.0f));
}

TEST_CASE("viewProjectionOf range la profondeur de 0 à 1, comme Vulkan et Direct3D 12")
{
    // Avec la convention d'OpenGL (glm::perspective), le plan proche tomberait à −1 : la moitié
    // proche de la scène sortirait du volume de vue.
    const Camera camera{.position = {0.0f, 0.0f, 0.0f},
                        .target = {0.0f, 0.0f, -1.0f},
                        .verticalFovRadians = glm::radians(60.0f),
                        .nearPlane = 0.1f,
                        .farPlane = 100.0f};
    const glm::mat4 viewProjection = viewProjectionOf(camera, 1.0f);

    CHECK(toNormalizedDevice(viewProjection, {0.0f, 0.0f, -0.1f}).z == doctest::Approx(0.0f));
    CHECK(toNormalizedDevice(viewProjection, {0.0f, 0.0f, -100.0f}).z == doctest::Approx(1.0f));
}
