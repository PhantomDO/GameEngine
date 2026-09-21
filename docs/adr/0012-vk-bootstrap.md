# ADR-0012 — vk-bootstrap pour créer le device Vulkan

- **Statut** : accepté le 2026-09-21 (choisi par Donnovan, sur la question posée en début de M1.2)
- **Date** : 2026-09-21
- **Milestone** : M1.2

## Contexte

NVRHI ne crée ni l'instance, ni le device, ni les queues Vulkan : l'application les lui fournit
([ADR-0002](0002-nvrhi.md)). L'ADR-0002 laissait pour M1.2 le choix de la manière de les créer.

Il faut une instance avec les extensions de surface demandées par SDL, les couches de validation en Debug,
un GPU qui gère Vulkan 1.3 avec `dynamicRendering`, `synchronization2` et les timeline semaphores (ce que NVRHI
exige), de préférence le GPU discret (la machine de référence a aussi un iGPU, SPECS §10), et une queue graphique
qui sait présenter sur la surface.

## Options envisagées

| Option | Pour | Contre |
|---|---|---|
| **vk-bootstrap** (MIT, vcpkg 1.4.350) | Sélection du GPU, vérification des versions et des fonctionnalités, queues, messager de debug : chaque exigence tient en une ligne qui se lit | Une dépendance de plus, et une couche entre nous et Vulkan |
| À la main, d'après Donut | Aucune dépendance ; le code de Donut sert de modèle ligne à ligne | Donut y consacre ~810 lignes (`DeviceManager_VK.cpp`, l. 102 à 913), avec beaucoup d'options dont on n'a pas besoin. Même réduit, plusieurs centaines de lignes de Vulkan à relire, ce que l'ADR-0002 voulait éviter |

## Décision

**vk-bootstrap**, confiné à `engine/gpu/src/` : il n'apparaît dans aucun en-tête public. Donut reste la
référence pour ce que vk-bootstrap ne fait pas : la swapchain, les frames en vol, et le branchement à NVRHI.

## Conséquences

- Les exigences envers le GPU se lisent comme une liste (`set_minimum_version(1, 3)`,
  `set_required_features_13(…)`) au lieu d'être dispersées dans des boucles d'énumération.
- vk-bootstrap charge lui-même la bibliothèque Vulkan : pas d'édition de liens avec le loader.
- En Debug, les couches de validation sont **exigées**, pas seulement demandées : sans elles, la création de
  l'instance échoue avec un message qui donne la commande d'installation (règle n°7).
- vk-bootstrap suit les versions de Vulkan (1.4.350 correspond aux en-têtes 1.4.350) : il se met à jour avec la
  baseline vcpkg, comme le reste.

## Ce que font les autres moteurs

Unreal (`VulkanRHI`, `FVulkanDynamicRHI::InitInstance`) et Godot (`RenderingContextDriverVulkan`) créent
instance et device à la main, avec leur propre énumération (**documenté** : sources publiques). Ils visent des
dizaines de plateformes et d'extensions, ce qui justifie ce code. Donut aussi, pour la même raison. vk-bootstrap
est surtout utilisé dans les projets plus petits et les tutoriels (vkguide.dev s'en sert).
