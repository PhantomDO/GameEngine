# `engine/gpu`

## Rôle

Donner un GPU au moteur : instance, surface et device Vulkan, puis le device NVRHI par-dessus. C'est, avec
`render/`, le seul module qui voit NVRHI, et avec `platform/`, le seul qui inclut SDL, pour créer la surface
(SPECS §7).

**État en M1.2** : device Vulkan et device NVRHI, validation redirigée vers nos logs (#12). La swapchain, le
redimensionnement et l'écran effacé arrivent avec #13.

## Invariants

1. **Ni Vulkan ni vk-bootstrap dans l'API.** `device.hpp` n'expose que NVRHI et `platform::Window` ;
   `VulkanContext` n'y est que déclaré. vk-bootstrap, les en-têtes Vulkan et SDL sont liés en `PRIVATE`.
2. **L'ordre de destruction est écrit dans l'ordre des déclarations.** Dans `GpuDevice`, `nvrhi` est déclaré
   après `vulkan` pour être détruit avant lui ; dans le sandbox, le `GpuDevice` est déclaré après la fenêtre pour
   que la surface disparaisse avant la fenêtre SDL qui la porte.
3. **En Debug, toute erreur de validation arrête le programme** sur une assertion, qu'elle vienne des couches
   Vulkan ou de NVRHI (règle n°4). Les messages du *loader* Vulkan, qui signale par exemple une couche tierce
   cassée (SPECS §10), sont journalisés sans arrêter : ce ne sont pas des bugs du moteur.
4. **Le dispatcher de Vulkan-Hpp est défini une seule fois**, dans `device_vk.cpp`.

## Points d'entrée

| Fichier | Contenu |
|---|---|
| [`include/levain/gpu/device.hpp`](include/levain/gpu/device.hpp) | `createGpuDevice`, `GpuDevice`, `DeviceOptions` |

## Ce que NVRHI fait pour nous, et ce qu'il ne fait pas

- **Il ne crée ni l'instance ni le device** : on les lui fournit, avec vk-bootstrap
  ([ADR-0012](../../docs/adr/0012-vk-bootstrap.md)). Il exige Vulkan 1.3 avec `dynamicRendering`,
  `synchronization2` et les timeline semaphores, et on ne lui annonce que les extensions réellement activées.
- **Une fois créé, il prend en charge le plus fastidieux** : allocation de la mémoire, barrières de
  synchronisation (il suit l'état de chaque ressource), destruction différée des ressources encore utilisées par
  le GPU, envois de données. On le verra à l'œuvre avec le premier triangle (M1.3).
- **Deux validations complémentaires** : les couches Vulkan vérifient ce que NVRHI envoie au pilote ; la couche de
  validation de NVRHI, qui enveloppe le device, vérifie l'usage qu'on fait de NVRHI lui-même.

Mesuré sur la machine de référence : device créé en 30 à 40 ms, validation comprise
(`SDL_VIDEO_DRIVER=x11 ./build/linux-debug/sandbox/levain_sandbox`).

## Pièges connus

| Piège | Parade |
|---|---|
| NVRHI est compilé en bibliothèque statique : il ne définit pas le dispatcher de Vulkan-Hpp et ne l'initialise pas | `VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE` et `VULKAN_HPP_DEFAULT_DISPATCHER.init(…)` dans `device_vk.cpp`, `VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1` dans le `CMakeLists.txt` |
| `nvrhi::vulkan::DeviceDesc desc;` laisse `transferQueue` et `computeQueue` indéterminés | `DeviceDesc desc{}` |
| LeakSanitizer signale 128 octets alloués par RADV | Faux positif : le loader décharge le pilote à la destruction de l'instance. Voir le skill `build`, `GOTCHA.md` |
| RADV affiche « radv is not a conformant Vulkan implementation » | Information de Mesa pour ce GPU récent (GFX1201), pas une erreur |

## Équivalents ailleurs

| Moteur | Module | Ce qu'on y trouve |
|---|---|---|
| **Unreal** | `VulkanRHI` | `FVulkanDynamicRHI` crée l'instance, `FVulkanDevice` le device, avec leur propre énumération des GPU et des extensions (**documenté** : sources publiques). |
| **Godot** | `drivers/vulkan` | `RenderingContextDriverVulkan` (instance, surfaces) et `RenderingDeviceDriverVulkan` (device) (**documenté** : dépôt public, depuis Godot 4.3). |
| **Unity** | — | Couche interne non publique. Ne pas supposer de correspondance. |
