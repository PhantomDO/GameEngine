# ADR-0003 — Fenêtre, input et plateforme : SDL3

- **Statut** : remplacé par [ADR-0010](0010-passage-a-rust.md) le 2026-09-20
- **Date** : 2026-09-20
- **Milestone** : M0.1

## Contexte

Il faut ouvrir une fenêtre, créer une surface Vulkan, lire le clavier, la souris et les manettes, sous Windows et
sous Linux (X11 et Wayland).

## Options envisagées

| Option | Pour | Contre |
|---|---|---|
| **SDL3** | Fenêtre, surface Vulkan, clavier, souris et manettes (avec base de mappings), X11 et Wayland, haute densité de pixels ; ABI stable depuis la 3.2 (janvier 2025) ; licence zlib ; très utilisé par l'industrie | Dépendance assez large |
| GLFW | Léger, simple | Support des manettes plus limité |
| Couche native (Win32, X11, Wayland) | Contrôle total | Trois backends à écrire pour une valeur d'apprentissage faible par rapport au coût |

## Décision

SDL3, confinée au module `platform/`. Le reste du moteur ne voit que nos propres types (événements, codes de
touches, fenêtre).

## Conséquences

- Remplacer SDL3 par une couche native plus tard ne touchera que `platform/`.
- NVRHI ne gère pas de fenêtre : SDL3 fournit la surface Vulkan (`SDL_Vulkan_CreateSurface`) et le handle
  de fenêtre Windows nécessaire à la swapchain D3D12. Donut utilise GLFW : son `DeviceManager` est adapté à SDL3.
- L'audio passe par miniaudio plutôt que par SDL3 : miniaudio fournit le mixage, le décodage et la spatialisation
  3D, alors que l'audio de SDL3 est plus bas niveau.

## Ce que font les autres moteurs

Unreal (`FGenericPlatformApplicationMisc`, `FPlatformApplication`) et Godot (`DisplayServer`) ont leur propre
couche par plateforme : ils visent consoles et mobiles, où SDL ne suffit pas toujours. Unity a aussi sa couche
interne.
