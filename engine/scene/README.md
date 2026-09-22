# `engine/scene`

## Rôle

Le modèle objet du moteur : le monde flecs, ses composants et ses systèmes (ADR-0004). Tout ce qui vit dans une
partie (objets, caméra, lumières) sera une entité de ce monde.

**État en M3.1** : les composants `Transform` et `Velocity`, et un système, `ApplyVelocity`, dans la phase
`OnUpdate`.

## Invariants

1. **flecs est visible ici et au-dessus, jamais en dessous** (SPECS §7) : ni dans `core`, `platform`, `gpu`, ni
   dans `render`, qui est sur une autre branche du graphe. Vérifié par le test `deps.flecs-visibility`. C'est
   l'application qui relie la scène et le rendu.
2. **La logique en fonctions libres, la glu flecs en une instruction** (ADR-0011) : `applyVelocity`
   (`motion.hpp`) ne sait rien de flecs et se teste seule ; `scene.cpp` ne fait que la brancher sur les entités.
3. **Les composants sont des données** : pas de méthode, pas de pointeur vers d'autres entités (les relations
   de flecs s'en chargeront, M3.2).

## Points d'entrée

| Fichier | Contenu |
|---|---|
| [`include/levain/scene/components.hpp`](include/levain/scene/components.hpp) | `Transform` (position, rotation, échelle), `Velocity` |
| [`include/levain/scene/motion.hpp`](include/levain/scene/motion.hpp) | `applyVelocity` — la logique, sans flecs |
| [`include/levain/scene/scene.hpp`](include/levain/scene/scene.hpp) | `SceneModule` — `world.import<levain::scene::SceneModule>()` |

## Trois notions de flecs

- **Archetype** : flecs range ensemble les entités qui ont exactement les mêmes composants, dans des tableaux
  contigus par composant. Un système parcourt ces tableaux sans sauter d'un objet à l'autre en mémoire : 100 000
  entités mises à jour en **0,09 ms** (`levain_scene_bench`, Release).
- **Requête** : un système déclare ce qu'il lit (`const Velocity`) et ce qu'il écrit (`Transform`) ; flecs trouve
  les archetypes qui correspondent et les garde en cache.
- **Phase** : `world.progress()` exécute les systèmes phase par phase, dans l'ordre du pipeline par défaut
  (`OnLoad`, `PostLoad`, `PreUpdate`, `OnUpdate`, `OnValidate`, `PostUpdate`, `PreStore`, `OnStore`). La
  simulation va dans `OnUpdate`.

Lectures : *Quickstart* de flecs (`docs/LECTURES.md`, D1), puis *Queries* (D2).

## Équivalents ailleurs

| Moteur | Où | Ce qu'on y trouve |
|---|---|---|
| **Unreal** | Mass Entity ; *tick groups* | Un ECS à archetypes à côté des Actors ; les phases s'appellent groupes de tick (`TG_PrePhysics`, `TG_PostPhysics`…) (**documenté** : documentation d'Epic). |
| **Unity** | Entities (DOTS) ; *system groups* | Un ECS à archetypes rangés en *chunks* ; les systèmes dans des groupes (`Initialization`, `Simulation`, `Presentation`) (**documenté** : manuel du package Entities). |
| **Godot** | Arbre de scène, `Node` | Pas d'ECS : des nœuds hiérarchiques, mis à jour par `_process` et `_physics_process` (**documenté** : docs officielles). |
