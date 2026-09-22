# ADR-0014 — Hot-reload des shaders : relancer le build

- **Statut** : accepté le 2026-09-22 (choisi par Donnovan sur sondage, M2.3)
- **Date** : 2026-09-22
- **Milestone** : M2.3

## Contexte

M2.3 demande de voir une modification de shader **en moins d'une seconde, sans redémarrer**, et qu'une erreur
de compilation **ne fasse pas planter** (issues #45 et #46). La ROADMAP prévoyait une recompilation « via la
bibliothèque Slang ». Avant d'écrire le code, il faut choisir qui compile le shader modifié, et comment le
sandbox s'aperçoit qu'il a changé.

Aujourd'hui, les shaders sont compilés au build : une commande CMake par point d'entrée, qui produit le SPIR-V
et le DXIL avec les options de NVRHI (décalages de registres, disposition des matrices ; ADR-0005,
`shaders/CMakeLists.txt`). Le moteur lit ces fichiers à la création des passes.

## Options envisagées

Mesures sur la machine de référence, `mesh.slang` modifié, trois essais chacune.

| Option | Pour | Contre |
|---|---|---|
| **Relancer le build** : `cmake --build <dossier> --target levain_shaders` dans un processus | Exactement les commandes du build, aucune option recopiée ; aucune dépendance (processus lancé par SDL) ; **330 à 400 ms** (SPIR-V et DXIL, deux points d'entrée en parallèle) | Outil de développement seulement : il faut CMake et le dossier de build ; le DXIL est recompilé aussi, sans servir |
| `slangc` en sous-processus | **145 ms** par point d'entrée, surtout le démarrage de `slangc` | Les options de compilation vivent à deux endroits, CMake et C++, à tenir d'accord : une divergence produit un shader qui compile mais se lie mal |
| Bibliothèque Slang dans le moteur (la ROADMAP) | En mémoire, sans processus ; le plus rapide une fois la bibliothèque chargée (non mesuré) | Nouvelle dépendance d'exécution (`libslang-compiler`, 28 Mo) ; API COM de Slang (sessions, modules, liaison), 150 à 200 lignes ; options recopiées aussi |

## Décision

**Le sandbox relance le build des shaders.** Il surveille les sources de `shaders/` par leur **date de
modification** (`std::filesystem::last_write_time`), à intervalle court. Quand l'une change, il lance
`cmake --build <dossier de build> --target levain_shaders` par `SDL_CreateProcess`, derrière une fonction de
`engine/platform`, et lit sa sortie :

- **échec** : la sortie de `slangc` va dans le log, les pipelines en place continuent de servir (issue #46,
  ADR-0008 : un shader invalide n'est pas un bug du moteur) ;
- **succès** : seules les passes qui utilisent le fichier modifié rechargent leurs shaders et recréent leur
  pipeline (issue #45). Binding layouts, buffers et binding sets restent en place.

Les dates de modification plutôt qu'`inotify` : une poignée de fichiers, une lecture de dates toutes les
quelques centaines de millisecondes, du C++ standard qui marchera tel quel sous Windows.

## Conséquences

- La ligne de la ROADMAP « recompilation à chaud via la bibliothèque Slang » est remplacée par ce choix.
- Le hot-reload n'existe que là où le dossier de build existe, c'est-à-dire en développement. Un jeu livré n'en
  a pas, comme un jeu Unreal cuisiné.
- Le délai vu par l'utilisateur : l'intervalle de surveillance, plus ~350 ms de build, plus la recréation du
  pipeline. Le critère d'une seconde laisse de la marge.
- **Limite connue** : un shader qui compile mais ne correspond plus à ce que la passe lui fournit (un attribut
  de sommet retiré, par exemple) produit une erreur de validation, donc une assertion en Debug (règle n°4). Le
  repli couvre les erreurs de compilation, pas les incohérences d'interface.
- À revoir quand la cuisson des assets arrivera (phase 4), ou si le nombre de shaders rend le build lent : la
  compilation pourra alors passer par un outil dédié, ou par la bibliothèque Slang.

## Ce que font les autres moteurs

- **Unreal** : les shaders sont compilés par des processus séparés, `ShaderCompileWorker`, que l'éditeur pilote ;
  la commande `recompileshaders changed` relance ceux dont la source a changé (**documenté** : documentation
  d'Epic sur le développement de shaders).
- **Unity** : la compilation passe par des processus `UnityShaderCompiler`, lancés par l'éditeur (**documenté** :
  manuel, « Shader compilation »).
- **Godot 4** : compile le GLSL en SPIR-V dans le processus du moteur, avec glslang (**documenté** : dépôt
  public, compilation des shaders de `RenderingDevice`).
