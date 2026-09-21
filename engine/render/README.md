# `engine/render`

## Rôle

Dessiner, avec NVRHI : pipelines, passes de rendu, et plus tard caméras, matériaux, éclairage et ombres
(SPECS §7). **Le module ne connaît que `nvrhi::IDevice`**, jamais Vulkan ni `engine/gpu` : il dessinera tel
quel sous Direct3D 12 le jour où ce backend existera (`docs/QA.md`, question du 2026-09-21).

**État en M2.1** : le premier triangle (`TrianglePass`), une caméra perspective (`Camera`).

## Invariants

1. **Aucune dépendance vers l'API graphique.** Le seul endroit qui distingue Vulkan de Direct3D 12 est
   `shaderExtensionFor` (`src/shader.cpp`), qui choisit entre SPIR-V et DXIL d'après `device.getGraphicsAPI()`.
2. **Les shaders sont compilés au build** (`shaders/CMakeLists.txt`, ADR-0005) et lus sur le disque à la création
   des passes. Une passe qui ne trouve pas ses shaders échoue avec un `Result`, pas une assertion : c'est le
   contenu du disque, pas un bug (ADR-0008).

## Points d'entrée

| Fichier | Contenu |
|---|---|
| [`include/levain/render/triangle.hpp`](include/levain/render/triangle.hpp) | `createTrianglePass`, `drawTriangle` |
| [`include/levain/render/camera.hpp`](include/levain/render/camera.hpp) | `Camera`, `viewProjectionOf` — profondeur de 0 à 1, comme Vulkan et Direct3D 12 |

## Ce qu'il faut pour dessiner un triangle avec NVRHI

1. **Deux shaders** (`nvrhi::IShader`), créés à partir du bytecode compilé au build.
2. **Un pipeline graphique** (`nvrhi::IGraphicsPipeline`) : les shaders, l'état de rendu (profondeur, faces à
   écarter), et le format des cibles où il dessinera. Il se crée une fois, pas à chaque frame.
3. **Un framebuffer** (`nvrhi::IFramebuffer`) : les textures cibles de ce draw. Ici, l'image de la swapchain.
4. **Un état graphique** passé à la command list (pipeline, framebuffer, viewport), puis un `draw` de 3 sommets.
   NVRHI place lui-même les barrières que ces changements d'état demandent.

Aucun vertex buffer : les sommets sont écrits dans le shader et choisis par `SV_VertexID`, comme le Basic
Triangle de Donut-Samples.

Mesuré sur la machine de référence, en Debug avec validation : **0,09 ms de CPU pour enregistrer et soumettre une
frame**, 0,15 ms pour la frame entière hors attente de l'écran
(`SDL_VIDEO_DRIVER=offscreen ./tools/tracy-capture.sh 3`, zones `commandes` et `rendu`).

## Équivalents ailleurs

| Moteur | Module | Ce qu'on y trouve |
|---|---|---|
| **Unreal** | `Renderer` | Les passes (`FDeferredShadingSceneRenderer`) écrites au-dessus de la RHI, via le Render Dependency Graph (**documenté** : sources publiques). |
| **Godot** | `servers/rendering/renderer_rd` | Les renderers Forward+ et Mobile, écrits au-dessus de `RenderingDevice` (**documenté** : dépôt public). |
| **Unity** | SRP (URP, HDRP) | Les pipelines de rendu, écrits en C# au-dessus de la couche graphique interne (**documenté** : packages publics). |
