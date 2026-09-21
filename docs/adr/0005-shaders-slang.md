# ADR-0005 — Langage de shaders : Slang

- **Statut** : accepté le 2026-09-20 ; remis en vigueur par [ADR-0011](0011-retour-au-cpp.md) après la parenthèse Rust ; amendé le 2026-09-21 (orchestration et DXIL, M1.3)
- **Date** : 2026-09-20
- **Milestone** : M0.1 (mise en place en M1.3)
- **Révision** : 2026-09-20, adaptation à NVRHI (ADR-0002)

## Contexte

Avec NVRHI (ADR-0002), il faut du SPIR-V pour Vulkan et du DXIL pour Direct3D 12. Il faut choisir le langage
source des shaders et son compilateur.

## Options envisagées

| Option | Pour | Contre |
|---|---|---|
| GLSL (glslang ou shaderc) | Langage natif de l'écosystème Vulkan ; utilisé par Godot | Pas de modules ni de génériques : les permutations passent par des `#define` |
| HLSL (DXC → SPIR-V et DXIL) | Standard de l'industrie (Unreal, Unity) ; chemin par défaut de NVRHI et Donut | Pas de système de modules ni de génériques |
| **Slang** | Syntaxe très proche du HLSL ; modules, génériques et interfaces ; compile vers SPIR-V et DXIL depuis une seule source ; hébergé par Khronos depuis fin 2024 ; fourni avec le Vulkan SDK de LunarG | Écosystème plus jeune ; moins d'exemples avec NVRHI que HLSL + DXC |

## Décision

Slang, compilé hors ligne au build vers les deux formats dont NVRHI a besoin :

- **SPIR-V** pour le backend Vulkan ;
- **DXIL** pour le backend Direct3D 12.

Les shaders suivent la convention de NVRHI : slots de type HLSL (`t`, `u`, `b`, `s`), avec des décalages de
binding appliqués pour Vulkan. L'orchestration de la compilation (ShaderMake, l'outil de NVIDIA qui accepte
Slang, ou de simples commandes CMake) est décidée en M1.3. La bibliothèque Slang liée au moteur sert au
hot-reload (M2.3).

## Conséquences

- Ce qu'on apprend reste transposable : Slang accepte l'essentiel du HLSL, le langage d'Unreal et d'Unity.
- Solution de repli peu coûteuse : HLSL compilé avec DXC, le chemin utilisé par Donut et les exemples NVRHI. On
  y bascule si Slang et les conventions de binding de NVRHI se combinent mal en M1.3.

## Amendement du 2026-09-21 — orchestration de la compilation (M1.3)

Décisions de Donnovan, sur question posée avant l'implémentation :

- **Commandes CMake et `slangc`**, pas ShaderMake : « le plus simple, et le moins à bricoler ». Une commande
  par point d'entrée (`shaders/CMakeLists.txt`), donc seul un shader modifié est recompilé, et une erreur de
  shader fait échouer le build avec le message de `slangc`. `slangc` vient du port vcpkg `shader-slang`, et
  CMake refuse d'en prendre un autre (`NO_DEFAULT_PATH`).
- **SPIR-V et DXIL dès maintenant**, conformément à la décision d'origine, bien que Direct3D 12 soit hors
  périmètre (ADR-0011). DXC vient du port `directx-dxc` ; faute de backend pour l'exécuter, chaque fichier DXIL
  est désassemblé par un test ctest, pour ne pas produire de fichiers que rien ne relit.

Conventions vérifiées en l'écrivant :

- **Décalages de binding de NVRHI sous Vulkan** : `-fvk-t-shift 0`, `-fvk-s-shift 128`, `-fvk-b-shift 256`,
  `-fvk-u-shift 384`, les valeurs par défaut de `nvrhi::VulkanBindingOffsets`.
- **Axe Y** : NVRHI inverse le viewport sous Vulkan (hauteur négative, `VKViewportWithDXCoords`). Les shaders
  suivent donc la convention de Direct3D sur les deux backends ; aucune option `-fvk-invert-y`.
- **`SV_VertexID`** : en HLSL, il compte depuis 0 sans le sommet de base du draw ; en Vulkan, il l'inclut.
  Slang compense en lisant ce sommet de base, ce qui exige `shaderDrawParameters` côté device (activé avec le
  premier triangle, #15, où la couche de validation l'a réclamé).

## Ce que font les autres moteurs

Unreal et Unity écrivent en HLSL et compilent vers chaque plateforme (Unreal via ShaderCompileWorker, Unity via
son système de variantes). Godot utilise un dialecte proche du GLSL.
