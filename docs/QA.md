# Questions et réponses

Les questions de Donnovan dont la réponse mérite d'être retrouvée. La plus récente en haut. Chaque réponse
renvoie au code, à un ADR ou à une source.

## Format

```
### Question (AAAA-MM-JJ, Mx.y)
Réponse courte, puis détails. Références : fichier:ligne, ADR, source externe.
```

---

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
Détails : [ADR-0001](adr/0001-langage-cpp20.md).

### GitHub, GitLab ou Azure DevOps ? (2026-09-20, M0.1)

GitHub en dépôt public : CI Windows + Linux gratuite et illimitée, board avec champs personnalisés, CLI `gh`
pilotable par Claude Code. Le seul vrai atout de GitLab est le suivi du temps natif, compensé ici par des champs
du board et le journal. Détails : [ADR-0006](adr/0006-hebergement-github.md).
