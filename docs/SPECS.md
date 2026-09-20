# Spécifications — Levain

> Version 0.4 — 20/09/2026 — statut : **validé par Donnovan** (ADR-0010 accepté)
> Documents liés : [ROADMAP](ROADMAP.md) · [JOURNAL](JOURNAL.md) · [ADR](adr/) · [Études](etudes/) ·
> [Lectures](LECTURES.md) · [Q&R](QA.md)
>
> v0.2 : couche graphique NVRHI (ADR-0002) et ECS flecs (ADR-0004) à la place d'une RHI et d'un ECS maison.
> v0.3 : C++23 au lieu de C++20 (ADR-0001 amendé), machine de référence renseignée, licence MIT, moteur nommé
> **Levain**.
> v0.4 : **passage à Rust** (ADR-0010). wgpu, bevy_ecs, winit remplacent NVRHI, flecs et SDL3.

## 1. Vision

**Faire un jeu avec un moteur 3D construit ensemble.** Le moteur est écrit en Rust, sur wgpu (Vulkan,
Direct3D 12, Metal) et bevy_ecs, et grandit par étapes mesurables.

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
| Langage | Rust, edition 2024 | [ADR-0010](adr/0010-passage-a-rust.md) |
| Couche graphique | wgpu (Vulkan, Direct3D 12, Metal) | [ADR-0010](adr/0010-passage-a-rust.md) |
| Fenêtre, input | winit | [ADR-0010](adr/0010-passage-a-rust.md) |
| Modèle objet | bevy_ecs (ECS à archétypes) | [ADR-0010](adr/0010-passage-a-rust.md) |
| Shaders | WGSL, validés par naga | [ADR-0010](adr/0010-passage-a-rust.md) |
| Hébergement et suivi | GitHub (dépôt public) | [ADR-0006](adr/0006-hebergement-github.md) |
| Build, tests, format, lint | cargo, `cargo test`, rustfmt, clippy | [ADR-0010](adr/0010-passage-a-rust.md) |
| Références de lecture | Bevy et les exemples wgpu | Lus, pas ajoutés en dépendance |
| Maths | glam | Conçu pour le jeu, conventions proches des shaders |
| Import glTF | gltf | Crate de référence de l'écosystème |
| Images | image, puis ktx2 | Simple d'abord, format GPU compressé ensuite |
| Physique | rapier3d | Physique Rust native, même famille que Jolt |
| Audio | kira | Pensé pour le jeu |
| UI de l'éditeur | egui | Mode immédiat, intégration wgpu native |
| Profiling | tracy-client | Même Tracy, client Rust |
| Logs | tracing | Standard de l'écosystème |
| Sérialisation | serde | Standard de l'écosystème |

## 7. Architecture cible

```
Cargo.toml          workspace
engine/
├── core/       types de base, logs, temps, fichiers
├── platform/   winit : fenêtre, événements, input brut
├── gpu/        device et surface wgpu, swapchain, cadence des frames
├── render/     renderer sur wgpu : caméras, matériaux, passes, éclairage, ombres
├── scene/      monde bevy_ecs, composants de base, transforms, hiérarchie
├── assets/     import, base d'assets (GUID), cuisson, cache, hot-reload
├── physics/    intégration rapier3d
├── audio/      intégration kira
├── input/      actions et axes au-dessus de platform
└── app/        boucle principale, cycle de vie
editor/         binaire de l'éditeur
sandbox/        une démo par milestone
games/          le jeu construit sur le moteur
shaders/        sources WGSL
tools/          scripts (bootstrap GitHub, mesures)
docs/           SPECS, ROADMAP, JOURNAL, LECTURES, QA, SETUP, adr/, etudes/
```

Chaque module est un crate du workspace. Les **tests unitaires vivent dans le crate** qu'ils testent
(`#[cfg(test)] mod tests`), les tests d'intégration dans `<crate>/tests/` : il n'y a pas de dossier `tests/`
à la racine, contrairement à l'arborescence C++ de la v0.3.

**Dépendances entre modules** (du bas vers le haut, jamais l'inverse) :

```
core ← platform ← gpu ← render
core ← scene (flecs) ← assets, physics, audio, input
tout ce qui précède ← app ← editor, sandbox, games
```

Visibilité des bibliothèques :

- winit : uniquement dans `platform/` (et `gpu/` pour la création de surface).
- wgpu : dans `gpu/`, `render/`, et `editor/` pour le rendu d'egui.
- bevy_ecs : c'est l'API du modèle objet, visible dans `scene/` et tout ce qui est au-dessus ; jamais dans
  `core/`, `platform/` ni `gpu/`.
- rapier3d : uniquement dans `physics/`.

`unsafe_code = "deny"` s'applique à tout le workspace. Les rares levées — frontières GPU — sont locales,
commentées, et repérables par un `grep` sur `allow(unsafe_code)`.

**Boucle principale (cible)** : simulation à pas fixe (60 Hz par défaut) avec accumulateur, exécutée par un
planning bevy_ecs dédié ; rendu à fréquence libre avec interpolation. Détails dans un ADR en M3.3.

## 8. Conventions

- **Langue** : identifiants, commits, logs et messages d'erreur en anglais (norme de l'industrie) ;
  documentation, ADR, études, journal, Q&R **et commentaires de code** en français. Les commentaires sont écrits
  pour Donnovan, seul relecteur, qui lit le français plus vite (ADR-0009). **Autre exception assumée** : le
  moteur s'appelle `Levain`, nom propre français, donc le namespace racine est `levain` et les cibles CMake sont
  préfixées `levain_`. Comme Godot, le nom ne se traduit pas.
- **Style C++** : variante mixte (types `PascalCase`, fonctions et variables `camelCase`, membres `m_`),
  accolades Allman, 4 espaces, 100 colonnes, décoration modérée. Détails et justifications dans
  [ADR-0009](adr/0009-style-cpp.md) ; appliqué mécaniquement par `.clang-format` et, pour le nommage, par
  clang-tidy.
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

- Le jeu : à choisir en M3.5, pour que les phases 4 à 8 servent ce jeu-là.
