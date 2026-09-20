# ADR-0005 — Langage de shaders : Slang

- **Statut** : remplacé par [ADR-0010](0010-passage-a-rust.md) le 2026-09-20
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

## Ce que font les autres moteurs

Unreal et Unity écrivent en HLSL et compilent vers chaque plateforme (Unreal via ShaderCompileWorker, Unity via
son système de variantes). Godot utilise un dialecte proche du GLSL.
