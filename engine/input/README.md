# `engine/input`

## Rôle

Traduire des appuis en **intentions de jeu**. Le code du jeu demande « avance » ou « saute », jamais « la
touche W » : c'est ce qui permet de changer les touches sans recompiler et de piloter la même action au clavier
et à la manette ([ADR-0017](../../docs/adr/0017-input-par-actions.md)).

**État** : les liaisons se lisent dans un fichier texte. L'état des actions et des axes vient juste après,
dans la même issue (#66), et la caméra libre du sandbox dans #67.

## Invariants

1. **Un nom inconnu échoue au chargement.** Une touche mal orthographiée ne doit pas donner une action qui ne
   répond jamais (règle n°7) : `loadBindings` rend une erreur, avec le numéro de ligne.
2. **Les noms sont ceux de SDL**, résolus par `platform/input.hpp` : aucune table à maintenir ici. Attention,
   la table de SDL est restée celle d'une manette Xbox — `a`, `b`, `x`, `y`, et non `south`, `east`…
3. **Une action ou un axe se résout en indice une fois**, au chargement (`actionIndex`, `axisIndex`) : aucune
   comparaison de chaînes par image.
4. **Ce module ne voit jamais SDL** : il ne connaît que les types de `platform`.

## Points d'entrée

| Fichier | Contenu |
|---|---|
| [`include/levain/input/bindings.hpp`](include/levain/input/bindings.hpp) | `Bindings`, `loadBindings`, `parseBindings` (la version qui se teste sans disque), `actionIndex`, `axisIndex` |
| [`../../data/input.cfg`](../../data/input.cfg) | Les liaisons de la démo. Un test vérifie qu'il est valide |

## Le fichier de liaisons

```
# une liaison par ligne, « # » commence un commentaire
action jump       = key:Space, pad:a
axis   move_right = key:D, key:A:-1, pad:leftx
axis   look_right = mouse:x:0.15, pad:rightx:90
deadzone = 0.15
```

- Un jeton s'écrit `appareil:nom[:échelle]`, l'appareil étant `key`, `mouse` ou `pad`.
- **L'échelle fait tout le travail** : elle transforme une touche en demi-axe (`key:A:-1`), règle la
  sensibilité de la souris (`mouse:x:0.15`) et la vitesse d'un stick, en degrés par seconde ici.
- `deadzone` est la zone morte des sticks, dont se servira l'état des axes.

## Ce qui n'est pas là, et pourquoi

Les **contextes** (jeu, menu, éditeur), les déclencheurs (appui long, double appui), les modificateurs, la
ré-assignation depuis l'interface et le rechargement à chaud du fichier sont **remis à plus tard**
(ADR-0017). Le jour où le jeu aura des menus, un contexte deviendra nécessaire : ce sera un ADR, pas une
rustine.

## Équivalents ailleurs

| Moteur | Où | Ce qu'on y trouve |
|---|---|---|
| **Unreal** | Enhanced Input | Des *Input Actions* (booléennes, 1D, 2D, 3D) liées dans des *Input Mapping Contexts* empilables, avec *triggers* et *modifiers* par liaison (**documenté** : documentation d'Epic). Notre modèle en est le socle. |
| **Unity** | Input System | Des *actions* rangées en *action maps*, des *control schemes* par appareil, le tout dans un asset JSON (**documenté** : manuel du package). |
| **Godot** | `InputMap` | Des actions nommées liées à des événements, rangées dans le fichier de projet, lues par `Input.is_action_pressed` et `Input.get_axis` (**documenté** : docs officielles). C'est le plus proche d'ici. |
