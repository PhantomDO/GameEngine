# Spécifications — Moteur 3D (nom à définir)

> Version 0.3 — 20/09/2026 — statut : **validé par Donnovan** (ADR 0001 à 0007 acceptés)
> Documents liés : [ROADMAP](ROADMAP.md) · [JOURNAL](JOURNAL.md) · [ADR](adr/) · [Études](etudes/) ·
> [Lectures](LECTURES.md) · [Q&R](QA.md)
>
> v0.2 : couche graphique NVRHI (ADR-0002) et ECS flecs (ADR-0004) à la place d'une RHI et d'un ECS maison.
> v0.3 : C++23 au lieu de C++20 (ADR-0001 amendé), machine de référence renseignée, licence MIT.

## 1. Vision

**Faire un jeu avec un moteur 3D construit ensemble.** Le moteur est écrit en C++23, sur NVRHI (Vulkan et
Direct3D 12) et flecs, et grandit par étapes mesurables.

La compréhension reste un objectif : chaque système est accompagné d'une note qui explique ce qu'il fait, pourquoi
il est conçu ainsi, et comment Unreal, Unity et Godot résolvent le même problème (et REEngine, Anvil ou Frostbite
quand des sources publiques existent : conférences GDC/CEDEC, blogs techniques). Quand on s'appuie sur une
bibliothèque, on la comprend **par la lecture** (études, lectures commentées) plutôt qu'en la réécrivant.

## 2. Rôles

| Qui | Rôle |
|---|---|
| **Donnovan** | Décideur et relecteur. Valide les specs et les ADR, relit le code, pose les questions, arbitre les compromis, choisit le jeu. Budget : **1 à 2 h par semaine**. |
| **Claude (Claude Code sur le PC de Donnovan)** | Concepteur-développeur. Écrit le code, les tests, la CI, la documentation et les études ; tient le board et le journal à jour. |

Le temps de Donnovan est la ressource rare du projet. Trois règles en découlent :

1. **Le code est écrit pour être lu.** Lisibilité avant astuce. Chaque module a un `README.md` (rôle, invariants,
   points d'entrée). Chaque PR contient un *guide de lecture* qui dit quoi lire, dans quel ordre, et pourquoi.
2. **Une PR se relit en 30 minutes au plus** (environ 400 lignes hors code tiers ou généré). Au-delà, on découpe.
3. **Une seule PR ouverte à la fois.** Claude n'attaque pas l'étape suivante tant que la précédente n'a pas été
   relue. Sans cette règle, le code avance plus vite que la compréhension.

## 3. Principes directeurs

1. **Ne pas recréer l'existant.** Quand une bibliothèque open-source reconnue couvre un domaine, on l'utilise :
   couche graphique (NVRHI), modèle objet (flecs), physique (Jolt), fenêtre et input (SDL3), UI d'outils (ImGui),
   audio (miniaudio), import glTF, profiling. On écrit ce qui fait **notre** moteur : l'assemblage et la boucle,
   l'initialisation des devices, le renderer et ses passes, le pipeline d'assets, l'input par actions, l'éditeur,
   et le jeu.
2. **Toujours un exécutable qui tourne.** Chaque milestone se termine par une démo lançable sur Windows et Linux.
3. **Mesurer.** Chaque milestone a des critères chiffrés (frame time, nombre d'entités, temps de chargement…),
   mesurés par une commande reproductible et consignés dans le journal.
4. **Tracer les décisions.** Toute décision structurante fait l'objet d'un ADR (`docs/adr/`) qui liste les
   alternatives écartées et pourquoi.
5. **Comparer.** Chaque phase produit une étude courte (`docs/etudes/`) : « comment les autres font ».
6. **Planifier par vagues.** Seules les deux prochaines phases sont découpées en issues détaillées ; les suivantes
   le sont à l'approche, avec ce qu'on aura appris.

## 4. Périmètre

### Dans le périmètre v1 (fin de la roadmap actuelle)

- **Plateformes** : Windows 10/11 (MSVC) et Linux (Clang), x86-64, dès le premier commit.
- **Rendu** : NVRHI avec backends Vulkan (Windows et Linux) et Direct3D 12 (Windows) ; rendu forward PBR, ombres
  en cascades, HDR et tonemapping, éclairage d'environnement (IBL), frustum culling, hot-reload des shaders.
- **Cœur** : boucle à pas fixe pour la simulation, ECS flecs, hiérarchie de transforms, input par actions,
  logs, assertions, allocateurs, profiling.
- **Assets** : import glTF 2.0, base d'assets à identifiants stables (GUID), cuisson (cooking) vers un format
  binaire, textures compressées KTX2, hot-reload.
- **Physique** : corps rigides, colliders, requêtes (raycasts), character controller.
- **Audio** : sons 2D et 3D.
- **Outils** : éditeur (hiérarchie, inspecteur, gizmos, picking), sérialisation de scènes, undo/redo, mode Play.
- **Le jeu** : un petit jeu 3D, choisi à la fin de la phase 3, construit uniquement avec le moteur et son éditeur.

### Hors périmètre v1 (candidats pour la v2)

Render graph, animation squelettique, GPU-driven rendering, ray tracing, scripting, réseau, streaming de monde,
consoles, mobile, macOS (NVRHI n'a pas de backend Metal), VR.

## 5. Contraintes

- **Coût zéro** : outils et bibliothèques gratuits, sous licence permissive (MIT, BSD, zlib, Apache 2.0, Boost).
- **Build reproductible** : un preset CMake et une commande de build, sans étape manuelle hormis l'installation
  des outils listés dans `docs/SETUP.md`.
- **CI verte obligatoire** sur Windows et Linux avant toute fusion dans `main`.
- **Zéro erreur de validation** en Debug : couche de validation NVRHI, validation layers Vulkan et couche de debug
  D3D12. Une erreur de validation est un bug bloquant.
- **Pas de code tiers copié dans le dépôt** : les dépendances passent par vcpkg, ou par `FetchContent` avec un
  commit figé quand il n'existe pas de port.

## 6. Stack technique

| Domaine | Choix | Justification |
|---|---|---|
| Langage | C++23, sans modules | [ADR-0001](adr/0001-langage-cpp23.md) |
| Couche graphique | NVRHI : Vulkan (Windows, Linux) et Direct3D 12 (Windows) | [ADR-0002](adr/0002-nvrhi.md) |
| Fenêtre, input, surface | SDL3 | [ADR-0003](adr/0003-plateforme-sdl3.md) |
| Modèle objet | flecs (ECS à archetypes) | [ADR-0004](adr/0004-ecs-flecs.md) |
| Shaders | Slang → SPIR-V et DXIL | [ADR-0005](adr/0005-shaders-slang.md) |
| Hébergement et suivi | GitHub (dépôt public) | [ADR-0006](adr/0006-hebergement-github.md) |
| Build et dépendances | CMake (presets) + Ninja + vcpkg (manifeste) | [ADR-0007](adr/0007-build-cmake-vcpkg.md) |
| Référence d'intégration NVRHI | Donut et Donut-Samples (NVIDIA, MIT) | Lus et adaptés, pas utilisés comme dépendance (ADR-0002) |
| Initialisation Vulkan | vk-bootstrap (optionnel) | Décidé en M1.2 |
| Maths | GLM | Conventions proches de celles des shaders |
| Import glTF | fastgltf | Rapide, C++ moderne |
| Images | stb_image, puis libktx (KTX2) | Simple d'abord, format GPU compressé ensuite |
| Physique | Jolt Physics | Utilisé par Horizon Forbidden West et Death Stranding 2, intégré à Godot 4.4 |
| Audio | miniaudio | Multiplateforme, spatialisation incluse |
| UI de l'éditeur | Dear ImGui (branche docking) + ImGuizmo | Standard des outils internes ; Donut fournit un renderer ImGui pour NVRHI |
| Profiling | Tracy | CPU et GPU, standard de l'industrie |
| Logs | spdlog | — |
| Tests | doctest | Léger, rapide à compiler |
| Sérialisation | JSON de flecs (addon meta) | Réflexion et JSON intégrés ; format binaire des assets en phase 4 |

## 7. Architecture cible

```
engine/
├── core/       types de base, logs, asserts, allocateurs, temps, fichiers
├── platform/   SDL3 : fenêtre, événements, input brut
├── gpu/        DeviceManager par backend (Vulkan, D3D12), swapchain, cadence des frames ; expose nvrhi::IDevice
├── render/     renderer sur NVRHI : caméras, matériaux, passes, éclairage, ombres
├── scene/      monde flecs, composants de base, transforms, hiérarchie, modules flecs
├── assets/     import, base d'assets (GUID), cuisson, cache, hot-reload
├── physics/    intégration Jolt
├── audio/      intégration miniaudio
├── input/      actions et axes au-dessus de platform
└── app/        boucle principale, cycle de vie
editor/         exécutable de l'éditeur
sandbox/        une démo par milestone
games/          le jeu construit sur le moteur
shaders/        sources Slang
tests/          tests unitaires et benchmarks
tools/          scripts (bootstrap GitHub, mesures)
docs/           SPECS, ROADMAP, JOURNAL, LECTURES, QA, SETUP, adr/, etudes/
```

**Dépendances entre modules** (du bas vers le haut, jamais l'inverse) :

```
core ← platform ← gpu ← render
core ← scene (flecs) ← assets, physics, audio, input
tout ce qui précède ← app ← editor, sandbox, games
```

Visibilité des bibliothèques :

- SDL3 : uniquement dans `platform/` (et `gpu/` pour la création de surface).
- NVRHI : dans `gpu/`, `render/`, et `editor/` pour le rendu d'ImGui.
- flecs : c'est l'API du modèle objet, visible dans `scene/` et tout ce qui est au-dessus ; jamais dans `core/`,
  `platform/` ni `gpu/`.
- Jolt : uniquement dans `physics/`.

**Boucle principale (cible)** : simulation à pas fixe (60 Hz par défaut) avec accumulateur, exécutée par un
pipeline flecs dédié ; rendu à fréquence libre avec interpolation. Détails dans un ADR en M3.3.

## 8. Conventions

- **Langue** : code, identifiants, commits et logs en anglais (norme de l'industrie) ; documentation, ADR, études,
  journal et Q&R en français.
- **Style** : clang-format et clang-tidy versionnés dans le dépôt et vérifiés en CI.
- **Commits** : [Conventional Commits](https://www.conventionalcommits.org/fr/) (`feat(render): …`, `fix(gpu): …`,
  `docs(adr): …`).
- **Branches** : `main` protégée ; une branche par issue (`m1.2/device-vulkan`) ; fusion par PR avec CI verte.
- **Licence** : MIT (fichier `LICENSE`), compatible avec le code adapté de Donut, qui garde son propre
  en-tête MIT.
- **Versions** : un tag par milestone terminé (`m1.3`), avec une GitHub Release qui contient les binaires de la
  démo produits par la CI et les mesures.

## 9. Définition de « terminé » pour un milestone

1. Démo `sandbox/` lançable sur Windows et Linux.
2. Critères chiffrés atteints, mesurés sur la machine de référence et consignés dans `docs/JOURNAL.md`.
3. CI verte, zéro erreur de validation.
4. `README.md` à jour dans chaque module touché.
5. Étude comparative écrite si la phase en prévoit une.
6. Temps estimé et temps passé renseignés sur le board GitHub.
7. Tag et GitHub Release publiés.

## 10. Machine de référence

Toutes les mesures de performance sont faites sur la même machine, pour que les chiffres soient comparables
d'un milestone à l'autre.

| Élément | Valeur |
|---|---|
| OS et version | CachyOS (Arch rolling), noyau 7.2.6-1-cachyos |
| CPU | AMD Ryzen 7 7800X3D (8 cœurs / 16 threads) |
| GPU | AMD Radeon RX 9070 XT (RDNA 4, GFX1201, `0x1002:0x7550`) |
| Pilote Vulkan | Mesa RADV 26.2.3-arch3.1 (`driverVersion` 26.2.3), Vulkan 1.4.354 sur le GPU, loader 1.4.357 |
| RAM | 16 Go (15,5 Go vus par le noyau) |
| Résolution de mesure | 1920×1080 |

Relevé le 20/09/2026 avec `vulkaninfo --summary`, `uname -r`, `/proc/cpuinfo` et `/proc/meminfo`.

Outils GPU sur cette machine : RenderDoc (captures), Tracy (profiling CPU et GPU), Radeon GPU Profiler (outil
du constructeur).

Deux points relevés par `vulkaninfo --summary`, à traiter en M1.2 :

1. **Deux GPU Vulkan** : le RX 9070 XT (`PHYSICAL_DEVICE_TYPE_DISCRETE_GPU`) et l'iGPU du 7800X3D
   (`RAPHAEL_MENDOCINO`, intégré). Le choix du device ne peut donc pas être « le premier de la liste » : il faut
   préférer le discret, avec une option de forçage.
2. **Couches Vulkan implicites installées par des outils tiers** (MangoHud, Steam overlay et fossilize, gamescope
   WSI, MAKO, Lossless Scaling). Celle de Lossless Scaling est cassée sur cette machine et le loader affiche une
   erreur à chaque `vkCreateInstance`. Ces messages ne viennent pas de notre code : les mesures et les sessions de
   debug se font avec `VK_LOADER_LAYERS_DISABLE=*` (hors validation layers, activées explicitement par le moteur).

### Vérification sous Windows

La machine de référence est sous Linux. Le code Windows est vérifié à trois niveaux :

1. **CI (à chaque PR)** : build MSVC, tests unitaires, et test de fumée D3D12 sous WARP (rendu logiciel).
2. **Proton sur la machine de référence (à chaque milestone de rendu)** : le binaire Windows produit par la CI est
   lancé sous Proton. Direct3D 12 y est traduit en Vulkan par vkd3d-proton : ça vérifie notre code Windows et le
   backend D3D12 de NVRHI sur le vrai GPU, mais pas un pilote D3D12 natif, et la couche de debug D3D12 de
   Microsoft n'y est pas disponible (seule la validation NVRHI s'applique).
3. **Une vraie machine Windows**, ponctuellement si l'occasion se présente. Non bloquant.

Une machine virtuelle Windows n'apporterait rien de plus : sans passthrough du GPU, elle n'a que du rendu
logiciel, comme WARP en CI.

## 11. Questions ouvertes

- Nom du moteur (et donc du dépôt).
- Le jeu : à choisir en M3.5, pour que les phases 4 à 8 servent ce jeu-là.
