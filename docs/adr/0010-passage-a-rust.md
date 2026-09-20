# ADR-0010 — Passage à Rust

- **Statut** : accepté le 2026-09-20
- **Date** : 2026-09-20
- **Milestone** : M0.2 (décidé à sa clôture)
- **Remplace** : [ADR-0001](0001-langage-cpp23.md) (langage), [ADR-0002](0002-nvrhi.md) (NVRHI),
  [ADR-0003](0003-plateforme-sdl3.md) (SDL3), [ADR-0004](0004-ecs-flecs.md) (flecs),
  [ADR-0005](0005-shaders-slang.md) (Slang), [ADR-0007](0007-build-cmake-vcpkg.md) (CMake + vcpkg).
  Remplace partiellement [ADR-0009](0009-style-cpp.md) (style).
- **Corrigé le 2026-09-20** : l'ADR-0003 manquait à cette liste alors que la décision remplace bien SDL3 par
  winit. Oubli relevé en appliquant l'ADR.

## Contexte

L'ADR-0001 retenait C++23 et se terminait par « on réévaluera en cours de route ». Donnovan a demandé la
réévaluation à la clôture de M0.2, au moment le moins coûteux possible : **107 lignes de code moteur**.

Deux prémisses de l'ADR-0001 ne tiennent plus :

1. **« Donnovan le lit vite. »** Vrai, mais il fait du C++ toute la semaine et n'a aucun besoin d'en pratiquer.
   Sa familiarité n'est plus un atout à préserver, c'est un acquis sans valeur marginale ici.
2. **« Donnovan apprendrait Rust au lieu d'apprendre les moteurs. »** L'argument supposait qu'il écrirait le
   code. Il en écrit ~1 % : il décide et il relit. Ce qui compte est donc sa vitesse de **lecture**, pas
   d'écriture.

L'ADR-0001 contenait aussi une erreur d'analyse : « le borrow checker résiste aux graphes d'objets et à la
mémoire gérée à la main ». C'est vrai en général et hors sujet ici, puisque la réponse Rust à ce problème est
l'ECS — que nous avions déjà choisi. Bevy le démontre.

## Méthode

Le choix n'a pas été fait par argumentation mais par lecture de code réel, en trois manches, comme pour
l'ADR-0009.

**Manche 1 — composant `Transform` et déplacement FPS.** Donnovan a trouvé la version Rust plus lisible **sans
avoir jamais lu de Rust**, et le C++ « beaucoup plus verbeux » alors qu'il en fait tous les jours. La différence
la plus nette : en Rust les dépendances d'un système sont dans sa signature
(`Res<InputState>`, `Query<(&mut Transform, &FpsController)>`), en C++ elles sont cachées dans le corps du
lambda (`it.world().get<InputState>()`).

**Manche 2 — propagation hiérarchique des transforms.** Cette manche a **favorisé le C++**. Le `cascade()` de
flecs fait le travail en dix lignes déclaratives ; la version Rust sûre en demande vingt, avec une pile
explicite. Bevy lui-même utilise `unsafe` (`Query::get_unchecked`) dans sa propagation, faute de pouvoir prouver
qu'une hiérarchie est un arbre ([bevy#4697](https://github.com/bevyengine/bevy/issues/4697)) — et l'a fait pour
la performance (8,1 ms → 1,88 ms), pas par nécessité.

**Manche 3 — build et CI.** Mesuré sur le dépôt réel à la fin de M0.2 :

| | C++ | Rust |
|---|---:|---:|
| `ci.yml` | 133 | **52** |
| Presets / profils | 101 | **0** |
| Déclaration des cibles | 29 | 42 |
| Dépendances | 13 | inclus |
| Format | 61 | **0** |
| Lint | 60 | **6** |
| **Total infrastructure** | **435** | **94** |

**435 lignes d'infrastructure pour 107 lignes de moteur.** Disparaissent : `vcvars`/`vswhere`, l'épinglage de
LLVM, le clone et le bootstrap de vcpkg, sa dépendance à `zip`, le couplage manuel `VCPKG_TAG` ↔
`builtin-baseline`, `/Zc:__cplusplus`, et la divergence `<ostream>` entre libstdc++ et la STL de Microsoft.
C'est-à-dire **l'intégralité des problèmes rencontrés pendant M0.2**.

## Décision

**Rust, edition 2024.** Nouvelle pile, chiffres de téléchargement relevés le 20/09/2026 sur crates.io :

| Rôle | Choix | Remplace | Téléchargements |
|---|---|---|---:|
| Couche graphique | **wgpu** 30 | NVRHI | 36 M |
| ECS | **bevy_ecs** 0.20 | flecs | 8,8 M |
| Fenêtre et entrées | **winit** 0.30 | SDL3 | 54 M |
| Maths | **glam** 0.33 | GLM | 144 M |
| Physique | **rapier3d** 0.35 | Jolt | 1,7 M |
| glTF | **gltf** 1.4 | fastgltf | 9,7 M |
| Shaders | **WGSL**, validés par naga | Slang | 40 M (naga) |
| UI d'éditeur | **egui** 0.36 | Dear ImGui | 24 M |
| Logs | **tracing** 0.1 | spdlog | 860 M |
| Profiling | **tracy-client** 0.19 | Tracy (C++) | 12 M |
| Audio | **kira** 0.12 | miniaudio | 0,9 M |
| Build, tests, format, lint | **cargo**, `cargo test`, rustfmt, clippy | CMake, vcpkg, doctest, clang-format, clang-tidy | — |

`bevy_ecs` et non `flecs_ecs` : le binding Rust de flecs existe et couvre relations, hiérarchies et pipelines,
mais il est **auto-déclaré alpha**, maintenu par une personne, totalise **11 359 téléchargements**, et son
`World` est `!Send`/`!Sync`. On ne pose pas le cœur du moteur là-dessus. Le prix est de perdre le `cascade()` de
la manche 2 — une vingtaine de lignes, dans un système, une fois.

## Conséquences

### Ce qu'on gagne

- **L'infrastructure divisée par 4,6**, et avec elle la classe entière de bugs de la manche 3.
- **`unsafe_code = "deny"` à l'échelle du workspace**, levé explicitement et localement aux frontières GPU.
- **M1.4 disparaît.** wgpu choisit son backend (Vulkan, Direct3D 12, Metal) : il n'y a plus de « backend D3D12 »
  à écrire. **1,0 h de budget Donnovan et une session rendues.**
- Les sanitizers, la couche de validation manuelle et la moitié des règles clang-tidy deviennent sans objet.

### Ce qu'on perd, et c'est réel

- **Le parcours de lecture Donut.** L'ADR-0002 et CLAUDE.md étaient bâtis sur « lire Donut et l'adapter ».
  Remplacé par les exemples wgpu et le crate `bevy_render`, qui sont bons mais constituent un autre programme.
- **L'objectif « comprendre Unreal, Unity, Godot » se paie plus cher.** Ils sont tous en C++ : chaque
  comparaison de README devient une traduction en plus. L'objectif est maintenu, le coût augmente.
- **Un an de C++ moteur que Donnovan n'écrira pas.** Assumé : il en fait toute la semaine.
- **wgpu n'est pas NVRHI.** Abstraction d'inspiration WebGPU, plus contrainte : le bindless et certaines
  fonctionnalités avancées y sont moins directs. À réévaluer si une limite bloque en phase 5.

### Sur le style

L'ADR-0009 avait deux moitiés. **La partie nommage devient sans objet** : rustfmt et les conventions Rust
(`snake_case` pour fonctions et variables, `CamelCase` pour les types) ne se configurent pas, et c'est un
avantage — le débat de style ne peut plus avoir lieu. **La partie « commentaires en français » est conservée**,
ainsi que SPECS §8 tel qu'amendé.

### Migration

Le C++ n'est pas effacé : le tag `m0.2` et l'historique git restent le témoin de ce qui a été construit. La
migration fait l'objet d'un nouveau milestone, **M0.4 — Socle Rust**, qui refait l'équivalent de M0.2 :
workspace cargo, CI, premier test, sandbox. Les milestones suivants sont retouchés dans la ROADMAP :
M1.2 (device wgpu), M1.4 (supprimé), M3.1 (bevy_ecs), M5.x (WGSL), M6.1 (rapier), M7.1 (egui), M8.1 (kira).

## Ce que font les autres moteurs

| Moteur | Langage | Source |
|---|---|---|
| Unreal, Godot, REEngine | C++ | dépôts et docs publics (documenté) |
| Unity | cœur C++, gameplay C# | documenté |
| **Bevy** | **Rust**, sur wgpu et `bevy_ecs` | dépôt public (documenté) |
| **Fyrox** | **Rust** | dépôt public (documenté) |

Nous quittons le camp majoritaire pour un camp minoritaire mais réel, avec deux moteurs complets comme
références de lecture. C'est assumé : le critère retenu est la vitesse de relecture d'un seul lecteur et le coût
d'outillage d'un projet à 1–2 h par semaine, pas la conformité à l'industrie.
