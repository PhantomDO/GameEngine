# Questions et réponses

Les questions de Donnovan dont la réponse mérite d'être retrouvée. La plus récente en haut. Chaque réponse
renvoie au code, à un ADR ou à une source.

## Format

```
### Question (AAAA-MM-JJ, Mx.y)
Réponse courte, puis détails. Références : fichier:ligne, ADR, source externe.
```

---

### Pourquoi « Levain » ? (2026-09-20, M0.1)

Parce que c'est le rythme du projet. Un levain se nourrit un peu chaque semaine, reste vivant entre deux
fournées, et sert de **base à partir de laquelle on cuit autre chose** — ce qu'est un moteur par rapport à un
jeu. Il se partage aussi, ce qui colle au « cuisiner à plusieurs » de Donnovan.

Le champ culinaire était déjà présent dans le projet avant le nom : M4.3 de la roadmap s'appelle
« Cuisson des assets » (*asset baking*).

Écartés et pourquoi : **Blitter** (la puce Amiga/Atari ST — Donnovan a commencé sur Game Boy Color et
PlayStation, la référence ne lui parlait pas) ; **Encore**, **Ludus**, **Noria**, **Marmite**, **Brigade**
(collisions GitHub sérieuses, dont `ludusavi`, un outil de sauvegardes de jeux — même domaine) ; **Braise**
(sémantiquement décroissante, elle pointe vers le passé) ; **Mijote** (verbe conjugué, mauvais namespace) ;
**Madeleine** et **Cartouche** (muets sur la technique).

Le namespace `levain` est une exception assumée à la règle « identifiants en anglais » (SPECS §8) : c'est un nom
propre, il ne se traduit pas, comme Godot.

### Le C++23 est-il stable en 2026 ? Peut-on y passer ? (2026-09-20, M0.1)

Oui côté Linux, « pas officiellement » côté Windows — et c'est gérable.

**Linux** : mesuré sur la machine de référence, Clang 22.1.8 et GCC 16.2.1 avec libstdc++ 16 donnent
`__cplusplus == 202302` et **toutes** les fonctionnalités C++23 sondées (dont `std::expected`, `std::print`,
`std::stacktrace`, `std::mdspan`, deducing this). Rien ne manque.

**Windows** : `/std:c++23` **n'existe pas**, ni en Visual Studio 2022 ni en Visual Studio 2026. Microsoft ne
propose que `/std:c++23preview` (« peut changer, peut ne pas être compatible en ABI d'une version à l'autre ») et
`/std:c++latest` (sur-ensemble qui déborde sur le brouillon C++26). CMake 4.4 mappe `CMAKE_CXX_STANDARD 23` vers
`-std:c++latest` chez MSVC (`/usr/share/cmake/Modules/Compiler/MSVC-CXX.cmake:46`). Les deux moitiés de la
matrice ne compilent donc pas le même langage.

**La parade** : c'est Linux qui fait autorité. La CI Linux compile en `-std=c++23 -pedantic-errors`, ce qui
refuse toute fonctionnalité post-C++23 (vérifié sur l'indexation de paquets `P2662`, refusée par Clang et par
GCC). Le code qui passe sous Linux est du C++23 ; MSVC, plus permissif, ne peut pas introduire de dérive
silencieuse.

Le déclencheur du changement est `std::expected` : la politique de gestion d'erreurs se décide en M0.3
(ADR-0008), et la trancher sans `std::expected` sous la main aurait appauvri le choix.

Détails, mesures et liste des trous MSVC restants : [ADR-0001](adr/0001-langage-cpp23.md).
Sources : [`/std` (msvc-180)](https://learn.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version?view=msvc-180),
[conformance C/C++ Microsoft](https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance).

### NVRHI est-il la norme de l'industrie ? Gère-t-il OpenGL et Metal ? (2026-09-20, M0.1)

Non aux deux. NVRHI est la couche de NVIDIA, utilisée par ses SDK RTX et ses exemples (Donut), et par quelques
projets tiers comme RBDOOM-3-BFG. Les grands moteurs ont chacun leur propre RHI (Unreal, Godot, Unity). Ses
backends sont Direct3D 11, Direct3D 12 et Vulkan : pas d'OpenGL ni de Metal. C'est sans conséquence pour nos
plateformes (Windows, Linux). On l'a choisi parce qu'il nous évite d'écrire une RHI, ce qui sert la priorité
« faire un jeu ». Détails : [ADR-0002](adr/0002-nvrhi.md), [étude E1](etudes/E1-rhi.md).

### NVRHI fonctionne-t-il sur une carte AMD ? (2026-09-20, M0.1)

Oui. NVRHI est une couche au-dessus de Vulkan et de Direct3D 12, deux API standard ; il tourne sur n'importe quel
GPU qui les prend en charge, dont la Radeon RX 9070 XT de la machine de référence. Seules les extensions propres à
NVIDIA (NVAPI, désactivée par défaut) sont réservées à ses cartes : on ne les utilise pas.

### Faut-il une VM Windows, ou Proton suffit-il ? (2026-09-20, M0.1)

Proton est plus utile : il lance le binaire Windows sur le vrai GPU, en traduisant Direct3D 12 en Vulkan
(vkd3d-proton). Une VM sans passthrough GPU n'a que du rendu logiciel, comme WARP en CI. Limite de Proton : ce
n'est pas un pilote D3D12 natif. Détails : SPECS §10.

### flecs ou EnTT ? (2026-09-20, M0.1)

flecs : documentation plus riche (quickstart, manuels par sujet, articles de l'auteur), explorer web, hiérarchies
et pipelines prêts à l'emploi, réflexion et JSON intégrés, utiles pour l'éditeur. EnTT est excellent et plus
minimal : il laisse davantage à écrire. Les deux sont bien maintenus. Détails : [ADR-0004](adr/0004-ecs-flecs.md).

### Faut-il utiliser un autre langage que le C++ pour un moteur en 2026 ? (2026-09-20, M0.1)

Non, pour ce projet. Rust, Zig et Odin sont de vraies alternatives, mais tous les moteurs étudiés sont en C++,
toutes les bibliothèques retenues aussi, et l'objectif est de comprendre les moteurs, pas d'apprendre un langage.
Détails : [ADR-0001](adr/0001-langage-cpp23.md).

### GitHub, GitLab ou Azure DevOps ? (2026-09-20, M0.1)

GitHub en dépôt public : CI Windows + Linux gratuite et illimitée, board avec champs personnalisés, CLI `gh`
pilotable par Claude Code. Le seul vrai atout de GitLab est le suivi du temps natif, compensé ici par des champs
du board et le journal. Détails : [ADR-0006](adr/0006-hebergement-github.md).
