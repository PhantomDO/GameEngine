# ADR-0007 — Build et dépendances : CMake (presets) + Ninja + vcpkg (manifeste)

- **Statut** : accepté le 2026-09-20 ; remis en vigueur par [ADR-0011](0011-retour-au-cpp.md) après la parenthèse Rust
- **Date** : 2026-09-20
- **Milestone** : M0.1 (mise en place en M0.2)

## Contexte

Il faut un build identique sous Windows et Linux, reproductible, avec une quinzaine de dépendances, et qui
fonctionne dans les IDE courants et en CI.

## Options envisagées

| Option | Pour | Contre |
|---|---|---|
| **CMake + vcpkg (manifeste)** | Standard de fait ; toutes nos dépendances sont dans vcpkg ; versions figées par une *baseline* ; cache binaire en CI | Premier build long (les dépendances sont compilées) |
| CMake + Conan | Gestionnaire puissant | Écosystème Python en plus ; configuration plus lourde |
| CMake + FetchContent ou CPM | Aucun outil externe | Recompile tout, gestion des versions moins nette |
| Sous-modules git | Contrôle total | Maintenance manuelle de chaque build tiers |
| Premake, Meson, xmake | Plus concis que CMake | Moins bien pris en charge par les IDE et les bibliothèques |

## Décision

- **CMake** avec `CMakePresets.json` : `linux-debug`, `linux-release`, `windows-debug`, `windows-release`.
- **Ninja** comme générateur sur les deux OS.
- **vcpkg en mode manifeste** (`vcpkg.json` + `builtin-baseline` figée).
- **Cache binaire vcpkg** en CI pour ne pas recompiler les dépendances à chaque exécution.

## Conséquences

- `compile_commands.json` est généré : clangd et clang-tidy fonctionnent sans configuration.
- Mettre à jour une dépendance revient à changer la baseline, dans une PR dédiée.
- NVRHI et flecs ont un port vcpkg. Si un outil n'en a pas (ShaderMake, par exemple), il passe par
  `FetchContent` avec un commit figé, jamais par une copie dans le dépôt.

## Ce que font les autres moteurs

Unreal : Unreal Build Tool (C#) et ses fichiers `.Build.cs`. Godot : SCons (Python). Unity : système interne.
Tous ont écrit leur propre outil pour gérer des centaines de modules et de plateformes. À notre échelle, CMake
suffit.
