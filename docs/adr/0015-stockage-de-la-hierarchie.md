# ADR-0015 — Hiérarchie de la scène : le composant `flecs::Parent`

- **Statut** : accepté le 2026-09-22 (choisi par Donnovan sur sondage, M3.2)
- **Date** : 2026-09-22
- **Milestone** : M3.2

## Contexte

M3.2 demande une hiérarchie parent-enfant et des matrices monde recalculées pour **100 000 entités sur 10
niveaux en moins de 2 ms** (issue #63). La ROADMAP prévoyait la relation `ChildOf` de flecs et une requête en
cascade, comme l'exemple « hierarchies » de flecs.

flecs 4.1 propose **deux stockages** pour la même hiérarchie, et le choix décide de la forme de tout le code qui
crée des entités (import glTF en M4.1, prefabs, éditeur) : il ne se change pas sans tout réécrire.

- `ChildOf` est une **relation** : chaque couple `(ChildOf, parent)` se comporte comme un composant, donc les
  enfants d'un parent donné vivent dans leur propre table. Mille parents font mille tables.
- `Parent` est un **composant** qui contient l'entité parente, et ne fragmente pas le stockage. flecs y ajoute
  une paire `(ParentDepth, profondeur)`, dont les requêtes se servent pour parcourir l'arbre par niveaux.

## Options envisagées

Mesures sur la machine de référence (SPECS §10, Release), 100 000 entités sur 10 niveaux, matrices monde
recalculées à chaque tour, médiane sur 500 tours. Deux formes d'arbre : des **chaînes** (10 000 racines, 9
descendants chacune, un seul enfant par parent) et un arbre **large** (10 000 entités par niveau, 10 enfants par
parent). Repère : les mêmes 100 000 entités sans hiérarchie, toutes racines, coûtent 1,05 ms par tour.

| Option | Chaînes | Arbre large | Pour | Contre |
|---|---:|---:|---|---|
| **`Parent` + groupes par profondeur** | **1,45 ms** (280 tables) | **1,43 ms** (280 tables) | Coût stable quelle que soit la forme de l'arbre ; une requête qui ne touche pas à la hiérarchie garde la vitesse du plat | Chaque entité lit la matrice de son parent par un accès aléatoire, et c'est l'essentiel du coût ; les opérateurs `Or` et `Not` ne le gèrent pas encore ; retirer un enfant coûte O(frères) |
| `ChildOf` + `cascade` (la ROADMAP) | 15,1 ms (180 264 tables) | 1,76 ms (18 264 tables) | Le code le plus simple : une requête, la matrice du parent lue **une fois par table** ; la voie documentée et montrée par les exemples | Une table par parent, **critère raté d'un facteur 7,5** sur les chaînes ; chaque table coûte ~160 ns à parcourir, et pèse sur la mémoire et sur les caches de toutes les requêtes |

Ces chiffres viennent de `levain_scene_bench`, qui garde les deux stockages mesurables côte à côte.

## Décision

**La hiérarchie de la scène passe par le composant `flecs::Parent`.** Un système unique, rangé par profondeur
(`group_by(flecs::ParentDepth)`), compose la matrice monde de chaque entité à partir de celle de son parent.

La ligne de la ROADMAP « relations `ChildOf`, matrices monde calculées par une requête en cascade » est
remplacée par ce choix. `cascade` reste la bonne réponse pour une hiérarchie `ChildOf` ; c'est le stockage qui
change, pas le principe du parcours par niveaux.

## Conséquences

- **Un enfant se crée par `world.entity(flecs::Parent{parent}, nom)`**, jamais par `.child_of(parent)` : une
  entité ne peut pas avoir les deux, et une entité rangée par `ChildOf` serait vue comme une racine par le
  système. Le README de `scene` le dit, et un test le vérifie.
- **Le tri par profondeur doit être demandé** : `group_by` seul range les tables par groupe mais parcourt les
  groupes dans l'ordre inverse de leur création. Sans le drapeau `EcsQueryGroupByOrdered`, un petit-enfant est
  calculé avant son parent et traîne une frame de retard. Un test le vérifie sur une hiérarchie construite dans
  le désordre.
- **Le coût est un accès aléatoire par entité** (la matrice du parent), pas un parcours de tableau contigu.
  C'est le prix du stockage non fragmenté ; il ne dépend pas de la forme de l'arbre, ce qui rend le temps de
  frame prévisible.
- **Les requêtes qui ne parlent pas de hiérarchie ne paient rien** : les entités restent rangées comme si elles
  étaient à plat. C'est ce qui compte pour le rendu et la physique.
- **À revoir** si une requête a besoin de `Or` ou de `Not` sur la hiérarchie, ou si un parent doit perdre des
  milliers d'enfants un par un (coût O(frères)) : flecs accepte les deux stockages dans le même monde, on
  pourrait n'en basculer qu'une partie.
- Pas de propagation paresseuse (« dirty flags ») : tout est recalculé à chaque tour. Le chiffre mesuré laisse
  la marge, et un drapeau par entité coûterait plus cher à tenir qu'à ignorer tant que la scène bouge.

## Ce que font les autres moteurs

- **Unity Entities (DOTS)** : le parent est un composant de l'enfant, `Parent`, et `LocalToWorldSystem` calcule
  `LocalToWorld` « en descendant récursivement chaque hiérarchie, en composant le `LocalTransform` de l'entité
  avec le `LocalToWorld` de son parent » (**documenté** : manuel du package Entities, « Transform concepts »).
  Même principe que le nôtre : un composant, pas un archetype par parent.
- **Bevy** : `ChildOf` est un composant qui « stocke l'entité parente », et sert à « propager la configuration
  ou les données héritées d'un parent, comme la visibilité ou les transforms monde » (**documenté** :
  documentation de l'API Bevy). Le nom dit relation, le stockage est un composant.
- **Unreal** : un `USceneComponent` est attaché à un parent, et sa transformation monde est recalculée en
  cascade quand le parent bouge (**documenté** : documentation d'Epic sur les Scene Components).
- **Godot** : `Node3D` garde sa transformation locale et calcule la globale à la demande, avec un drapeau
  « sale » posé sur les enfants quand un parent bouge (**déduit** du code de `Node3D`, `_propagate_transform_changed`).

## Sources

1. flecs, *Hierarchies Manual*, section « Hierarchy storage » —
   https://www.flecs.dev/flecs/md_docs_2HierarchiesManual.html
2. flecs, *Queries Manual*, section « Grouping » — https://www.flecs.dev/flecs/md_docs_2Queries.html
3. Unity, *Transform concepts* (package Entities) —
   https://docs.unity3d.com/Packages/com.unity.entities@1.3/manual/transforms-concepts.html
4. Bevy, `ChildOf` — https://docs.rs/bevy/latest/bevy/prelude/struct.ChildOf.html
