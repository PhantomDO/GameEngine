# Journal de bord

Une entrée par session de travail, la plus récente en haut. Les heures Donnovan sont celles qu'il déclare ;
les chiffres de performance viennent de commandes versionnées, sur la machine de référence.

## Format d'une entrée

```
## AAAA-MM-JJ — Mx.y — titre court
- Temps Donnovan : x h (estimé y h)
- Sessions Claude Code : n
- Fait : …
- Mesures : critère → valeur (commande)
- Décisions : ADR-XXXX …
- Écarts et problèmes : …
- Prochaine étape : …
```

## Cumul

| Phase | Estimé (h) | Passé (h) | Ratio |
|---|---:|---:|---:|
| 0 | 4,0 | — | — |
| 1 | 5,5 | — | — |

---

## 2026-09-20 — M0.1 — Dépôt local, modèles GitHub et machine de référence

- Temps Donnovan : à renseigner
- Sessions Claude Code : 1
- Fait : modèles d'issue et de PR déplacés de `tools/github-templates/` vers `.github/` ; dépôt git local initialisé
  (`main`) et premier commit ; SPECS §10 complété à partir de la machine.
- Mesures : machine de référence → CachyOS noyau 7.2.6-1-cachyos, Ryzen 7 7800X3D, RX 9070 XT (GFX1201),
  Mesa RADV 26.2.3-arch3.1, Vulkan 1.4.354, 16 Go (`vulkaninfo --summary`, `uname -r`, `/proc/cpuinfo`,
  `/proc/meminfo`). Outils présents : CMake 4.4.3, Ninja 1.13.2, Clang 22.1.8, GCC 16.2.1, git 2.55.0, gh 2.101.0.
- Écarts et problèmes : deux GPU Vulkan sur la machine (le discret et l'iGPU du 7800X3D) — la sélection de device
  en M1.2 devra préférer le discret ; plusieurs couches Vulkan implicites tierces sont installées et celle de
  Lossless Scaling est cassée (erreur du loader à chaque `vkCreateInstance`). Les deux points sont notés
  dans SPECS §10.
- Prochaine étape : `./tools/github-bootstrap.sh GameEngine` (dépôt public, labels, 35 milestones, board, issues
  des phases 0 et 1), après relecture du dry-run par Donnovan ; puis M0.2.

## 2026-09-20 — M0.1 — Machine de référence et vérification Windows

- Temps Donnovan : à renseigner
- Fait : machine de référence renseignée (CachyOS, Ryzen 7 7800X3D, Radeon RX 9070 XT, 16 Go) ; versions d'OS et
  de pilote à compléter avec `vulkaninfo --summary`.
- Décision : pas de machine Windows ; vérification en trois niveaux (CI + WARP, binaire Windows sous Proton sur la
  machine de référence, vraie machine Windows ponctuellement). Une VM Windows écartée : sans passthrough GPU, elle
  n'apporte rien de plus que WARP.
- Dépôt GitHub : pas encore créé ; Claude Code le créera avec `tools/github-bootstrap.sh` après la connexion de
  Donnovan à `gh`. Nom provisoire : GameEngine.

## 2026-09-20 — M0.1 — Révision : NVRHI et flecs

- Temps Donnovan : à renseigner (échange vocal + relecture)
- Sessions Claude Code : 0
- Contexte : Donnovan précise que **la priorité est de faire un jeu avec notre moteur**, et qu'apprendre Vulkan en
  détail ne l'intéresse pas.
- Décisions proposées : NVRHI à la place d'une RHI maison, backends Vulkan + Direct3D 12 (ADR-0002, réécrit) ;
  flecs plutôt qu'EnTT ou un ECS maison, pour sa documentation, son explorer, ses hiérarchies, ses pipelines et sa
  réflexion JSON (ADR-0004, réécrit) ; Slang compilé en SPIR-V et DXIL (ADR-0005, révisé).
- Correction apportée : NVRHI n'est pas la norme de l'industrie (Unreal, Unity et Godot ont leur propre RHI) et
  ne gère ni OpenGL ni Metal (backends : D3D11, D3D12, Vulkan).
- Roadmap v0.2 : 77 h → 68 h Donnovan, 55 → 49 sessions, 35 milestones ; « RHI mince » supprimé ; ajout de M1.4
  (backend D3D12) et M3.5 (choix du jeu) ; le jeu jouable visé début août 2027 à 1,5 h/semaine.
- Ajouts : étude E1 (couches RHI) écrite en avance, `docs/LECTURES.md` (lectures commentées).
- Prochaine étape : inchangée (validation des specs, puis création du dépôt et M0.2).

## 2026-09-20 — M0.1 — Specs initiales

- Temps Donnovan : à renseigner (estimé 1,0 h pour tout M0.1)
- Sessions Claude Code : 0 (rédaction dans une conversation claude.ai)
- Fait : SPECS v0.1, ROADMAP v0.1 (77 h Donnovan, 55 sessions, 34 milestones), ADR 0001 à 0007, CLAUDE.md,
  modèles d'issue et de PR, script `tools/github-bootstrap.sh`.
- Décisions proposées : C++20 sans modules, Vulkan 1.3 avec RHI maison, SDL3, ECS maison à sparse sets, Slang,
  GitHub public, CMake + vcpkg.
- Prochaine étape : Donnovan valide ou amende les specs et les ADR, renseigne la machine de référence, choisit
  le nom ; puis création du dépôt avec le script et démarrage de M0.2 dans Claude Code.
