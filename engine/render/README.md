# `engine/render`

## Rôle

Dessiner, avec NVRHI : pipelines, passes de rendu, et plus tard caméras, matériaux, éclairage et ombres
(SPECS §7). **Le module ne connaît que `nvrhi::IDevice`**, jamais Vulkan ni `engine/gpu` : il dessinera tel
quel sous Direct3D 12 le jour où ce backend existera (`docs/QA.md`, question du 2026-09-21).

**État en M2.1** : le premier triangle (`TrianglePass`), une caméra perspective (`Camera`), des meshes indexés
dessinés avec un depth buffer (`MeshPass`), en plusieurs exemplaires par un seul draw (`Instances`), et le temps
GPU d'une frame (`GpuTimer`).

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
| [`include/levain/render/mesh.hpp`](include/levain/render/mesh.hpp) | `Mesh`, `createMesh`, `createCube` — buffers de sommets et d'indices ; `Instances`, `createInstances` — un décalage par exemplaire |
| [`include/levain/render/mesh_pass.hpp`](include/levain/render/mesh_pass.hpp) | `createMeshPass`, `ensureDepthTexture`, `drawMesh` — la première passe avec constantes et profondeur |
| [`include/levain/render/gpu_timer.hpp`](include/levain/render/gpu_timer.hpp) | `GpuTimer`, `beginGpuTimer`, `endGpuTimer` — temps GPU par timer queries |

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

## Ce qu'ajoute un mesh au triangle

- **Des buffers de sommets et d'indices**, envoyés par `writeBuffer` : NVRHI passe par un buffer d'envoi interne et
  place les barrières. Un *input layout* dit au pipeline comment lire chaque sommet (position, couleur).
- **Des constantes par frame** dans un *volatile constant buffer*, lié par un binding set dans `space0`
  (ADR-0013) : NVRHI fournit une nouvelle version à chaque écriture, sans buffer par frame en vol à gérer.
- **Un depth buffer**, recréé seulement quand la taille de l'image change, et l'élimination des faces arrière
  (sens trigonométrique, vérifié par le test de fumée du cube).

## Ce qu'ajoute l'instancing

- **Un second vertex buffer, lu par exemplaire et non par sommet** : l'attribut `INSTANCE_OFFSET` est déclaré
  `setIsInstanced(true)` dans l'input layout et lu dans le slot 1. Le GPU avance d'un élément à chaque exemplaire.
- **Un seul `drawIndexed`** avec `instanceCount = 10 000` : le CPU enregistre un appel, quel que soit le nombre
  de cubes. Mesuré : 0,022 ms de GPU pour 10 000 cubes en 1080p (journal, #42).
- Le décalage est une simple position, pas une matrice : c'est tout ce dont la grille a besoin. Une matrice par
  exemplaire viendra avec des objets qui tournent chacun de leur côté.

## Mesurer le temps GPU

Le CPU ne voit que le temps qu'il passe à enregistrer : le GPU exécute plus tard, en parallèle. Une **timer
query** demande au GPU d'horodater le début et la fin d'un bloc de commandes ; NVRHI la crée, l'enregistre
(`beginTimerQuery`, `endTimerQuery`) et rend la durée en secondes (`getTimerQueryTime`). Le résultat n'arrive
que quand le GPU a fini la frame, d'où l'anneau de trois requêtes de `GpuTimer`.

## Équivalents ailleurs

| Moteur | Module | Ce qu'on y trouve |
|---|---|---|
| **Unreal** | `Renderer` | Les passes (`FDeferredShadingSceneRenderer`) écrites au-dessus de la RHI, via le Render Dependency Graph (**documenté** : sources publiques). |
| **Godot** | `servers/rendering/renderer_rd` | Les renderers Forward+ et Mobile, écrits au-dessus de `RenderingDevice` (**documenté** : dépôt public). |
| **Unity** | SRP (URP, HDRP) | Les pipelines de rendu, écrits en C# au-dessus de la couche graphique interne (**documenté** : packages publics). |
| **Unreal** | `FRHIRenderQuery`, `stat gpu` | Timer queries au-dessus de la RHI, affichées par passe (**documenté** : sources publiques). |
| **Unity** | GPU Instancing, Frame Timing Manager | Instancing activé par matériau ; temps GPU par frame (**documenté** : manuel). |
| **Godot** | `MultiMeshInstance3D` | Un mesh dessiné en N exemplaires par un seul draw (**documenté** : docs officielles). |
