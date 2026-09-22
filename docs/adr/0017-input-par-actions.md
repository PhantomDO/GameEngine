# ADR-0017 — Input par actions : un fichier de liaisons, les noms de SDL

- **Statut** : proposé le 2026-09-22 (M3.4)
- **Date** : 2026-09-22
- **Milestone** : M3.4

## Contexte

M3.4 demande deux choses (issues #66 et #67) : **une même action pilotée au clavier et à la manette**, et
**changer les touches sans recompiler**. Aujourd'hui, `platform/` ne remonte que les événements de fenêtre
(`window.hpp`) ; rien du clavier, de la souris ou des manettes n'existe.

Trois questions se posent ensemble :

1. **Qui traduit un appui en intention de jeu ?** Le code de la caméra ne doit pas connaître la touche W — sinon
   « changer les touches » veut dire recompiler, et « la même action à la manette » veut dire écrire le code
   deux fois.
2. **Où vivent les noms** des touches, boutons et axes, et qui les lit depuis un fichier ?
3. **Jusqu'où va le modèle** ? Unreal et Unity ont chacun un système d'input plus gros que notre moteur entier.

## Options envisagées

### Le modèle

| Option | Pour | Contre |
|---|---|---|
| **Actions (oui/non) et axes (valeur continue), liés dans un fichier** | Le code du jeu demande « avance » et « regarde », jamais « la touche W ». Les deux critères tombent d'eux-mêmes. C'est le socle commun d'Unreal, d'Unity et de Godot | Une indirection de plus entre l'appui et l'effet |
| Lire l'état du clavier directement dans le jeu | Rien à écrire | Les deux critères sont ratés : les touches sont dans le code, et la manette demande un second chemin |
| Le modèle complet d'entrée d'Unreal (contextes, déclencheurs, modificateurs) | Tout est possible : combos, appui long, priorités entre contextes | Hors budget de M3.4, et rien ne le demande encore : la démo a une caméra |

### Le fichier de liaisons

| Option | Pour | Contre |
|---|---|---|
| **Un fichier texte ligne à ligne, lu par un parseur d'une soixantaine de lignes** | Se lit et se modifie à la main sans outil ; aucune dépendance ; les **noms des touches et des boutons sont ceux de SDL**, résolus par `SDL_GetScancodeFromName` et consorts — aucune table à maintenir de notre côté | Un format de plus dans le projet, qu'il faudra documenter |
| JSON, décodé par la réflexion de flecs (celle de l'explorer) | Aucun parseur à écrire pour la structure ; format connu de tous | Il faut décrire à flecs `std::string` et `std::vector` en types opaques (~40 lignes de glu), **et** garder un petit analyseur pour les jetons `key:Space` : le JSON ne supprime pas le travail, il le déplace |
| Une bibliothèque JSON ou TOML (vcpkg) | Robuste, éprouvé | Une dépendance d'exécution pour lire une dizaine de lignes, alors qu'`ADR-0007` demande de justifier chaque ajout |

## Décision

**Actions et axes, liés dans un fichier texte dont les noms viennent de SDL.**

```
# data/input.cfg — une liaison par ligne, « # » commence un commentaire
action jump       = key:Space, pad:a
axis   move_right = key:D, key:A:-1, pad:leftx
axis   look_up    = mouse:y:-0.08, pad:righty:-1
deadzone = 0.15
```

- Un **jeton** s'écrit `appareil:nom[:échelle]`. L'échelle vaut 1 par défaut ; c'est elle qui fait d'une touche
  un axe (`key:A:-1`) et qui règle la sensibilité de la souris.
- Les noms sont **ceux de SDL**, vérifiés : `Space`, `D`, `Left Shift` pour les touches ; `a`, `b`, `start`
  pour les boutons ; `leftx`, `righty`, `righttrigger` pour les axes. Attention, la table de noms de SDL est
  restée celle d'une manette Xbox : `SDL_GetGamepadButtonFromString("south")` rend −1, alors que
  l'énumération, elle, s'appelle `SDL_GAMEPAD_BUTTON_SOUTH`. Un nom refusé est une erreur de chargement.
- La souris n'a pas de table de noms chez SDL : `mouse:x` et `mouse:y` sont à nous, et ne désignent que le
  mouvement **relatif**.
- `platform/` gagne **l'input brut** : les appuis, les mouvements de souris et les axes de manette, dans nos
  propres types (aucun en-tête SDL ne sort du module, invariant n°1). Il gagne aussi la **résolution des noms**
  (`keyFromName("Space")`, `padButtonFromName("South")`, `padAxisFromName("LeftX")`), parce que ces tables sont
  celles de SDL et que SDL ne se voit que là.
- **`engine/input/`** (prévu par SPECS §7) lit le fichier, tient l'état des actions et des axes, et répond à
  `actionPressed(state, handle)` / `axisValue(state, handle)`. Une action ou un axe se résout **une fois**, au
  chargement, en un indice : pas de comparaison de chaînes par frame.
- Le jeu n'appelle jamais `platform` : il demande « avance », « regarde », « saute ».

## Conséquences

- **Changer les touches ne recompile rien** : le fichier est lu au démarrage (et plus tard rechargé à chaud,
  comme les shaders).
- **Une action inconnue dans le fichier est une erreur bruyante** (règle n°7) : un nom mal écrit ne doit pas
  donner une action qui ne répond jamais. Le chargement rend une erreur, pas un silence.
- **Le clavier et la manette passent par le même chemin** : la caméra de #67 ne saura pas ce qui la pilote.
  C'est le critère, et c'est aussi ce qui permettra de rejouer une partie à partir des actions.
- **Le mouvement de la souris est un axe comme un autre**, avec son échelle dans le fichier. La caméra
  demandera la souris relative à SDL (`SDL_SetWindowRelativeMouseMode`) quand le bouton droit est tenu.
- **Ce qui est remis à plus tard**, et marqué comme tel dans le code : les **contextes** (jeu, menu, éditeur),
  les déclencheurs (appui long, double appui), les modificateurs, la ré-assignation depuis l'interface, et le
  rechargement à chaud du fichier. Le jour où le jeu aura des menus, un contexte deviendra nécessaire : ce sera
  un ADR, pas une rustine.
- **Le fichier vit dans `data/`** avec les textures, et sera un asset comme un autre en phase 4.

## Ce que font les autres moteurs

- **Unreal (Enhanced Input)** : des *Input Actions* (booléennes, 1D, 2D, 3D) liées dans des *Input Mapping
  Contexts* empilables, avec des *triggers* et des *modifiers* par liaison (**documenté** : documentation
  d'Epic sur Enhanced Input). Notre modèle est son socle, sans les contextes ni les déclencheurs.
- **Unity (Input System)** : des *actions* rangées en *action maps*, des liaisons par appareil et des *control
  schemes*, le tout dans un asset JSON (**documenté** : manuel du package Input System).
- **Godot** : une `InputMap` d'actions nommées, chacune liée à des événements, rangée dans le fichier de
  projet, et lue par `Input.is_action_pressed` / `Input.get_axis` (**documenté** : documentation de Godot).
  C'est le plus proche de ce que fait cet ADR.

## Sources

1. SDL3, `SDL_GetScancodeFromName`, `SDL_GetGamepadButtonFromString`, `SDL_GetGamepadAxisFromString` —
   https://wiki.libsdl.org/SDL3/
2. Epic Games, *Enhanced Input in Unreal Engine* —
   https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine
3. Unity, *Input System* — https://docs.unity3d.com/Packages/com.unity.inputsystem@1.14/manual/index.html
4. Godot, *Using InputEvent* et *InputMap* —
   https://docs.godotengine.org/en/stable/tutorials/inputs/inputevent.html
