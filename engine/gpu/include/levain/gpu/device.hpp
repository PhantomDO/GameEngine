#pragma once

#include <memory>

#include <nvrhi/nvrhi.h>

#include "levain/core/error.hpp"
#include "levain/platform/window.hpp"

namespace levain::gpu
{

/// Ce que NVRHI ne crée pas lui-même : instance, surface et device Vulkan. Défini dans
/// `device_vk.cpp` : ni Vulkan ni vk-bootstrap n'apparaissent dans cet en-tête.
struct VulkanContext;

/// Détruit device, surface et instance, dans cet ordre.
struct VulkanContextDeleter
{
    void operator()(VulkanContext* context) const noexcept;
};

struct DeviceOptions
{
    /// Couches de validation Vulkan et couche de validation NVRHI, exigées en Debug (règle n°4).
    /// Une erreur de l'une ou de l'autre arrête le programme sur une assertion.
    bool enableValidation = false;
};

/// Le GPU vu par le moteur : un `nvrhi::IDevice`, et les objets Vulkan qui le portent.
///
/// **L'ordre des membres est l'ordre de destruction inverse.** `nvrhi` est déclaré après `vulkan`
/// pour disparaître avant lui : détruit après, il libérerait ses ressources sur un VkDevice déjà
/// détruit. Pour la même raison, ne gardez pas de `nvrhi::DeviceHandle` plus longtemps que le
/// `GpuDevice` d'où il vient.
struct GpuDevice
{
    std::unique_ptr<VulkanContext, VulkanContextDeleter> vulkan;
    nvrhi::DeviceHandle nvrhi;
};

/// Crée le device Vulkan sur le GPU le plus adapté (discret de préférence), puis le device NVRHI
/// par-dessus. Le nom du GPU et la version du pilote sont journalisés dans la catégorie `gpu`.
///
/// Échoue sans GPU compatible (Vulkan 1.3, dynamicRendering, synchronization2, timeline
/// semaphores), ou si la validation est demandée sans que ses couches soient installées.
[[nodiscard]] core::Result<GpuDevice> createGpuDevice(const platform::Window& window,
                                                      const DeviceOptions& options);

} // namespace levain::gpu
