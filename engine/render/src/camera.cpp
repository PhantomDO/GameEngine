#include "levain/render/camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace levain::render
{

glm::mat4 viewProjectionOf(const Camera& camera, float aspectRatio)
{
    // RH_ZO : repère droitier, profondeur de 0 à 1 comme Vulkan et Direct3D 12. glm::perspective
    // suppose celle d'OpenGL (−1 à 1) : la moitié proche de la scène sortirait du volume de vue.
    const glm::mat4 projection = glm::perspectiveRH_ZO(camera.verticalFovRadians, aspectRatio,
                                                       camera.nearPlane, camera.farPlane);
    const glm::mat4 view =
        glm::lookAtRH(camera.position, camera.target, glm::vec3{0.0f, 1.0f, 0.0f});
    return projection * view;
}

} // namespace levain::render
