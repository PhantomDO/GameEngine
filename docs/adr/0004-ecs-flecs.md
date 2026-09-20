# ADR-0004 — Modèle objet : ECS avec flecs

- **Statut** : accepté le 2026-09-20
- **Date** : 2026-09-20
- **Milestone** : M0.1 (intégration en M3.1)
- **Historique** : la première version proposait d'écrire notre propre ECS. Avec la priorité donnée au jeu,
  Donnovan a demandé de comparer flecs et EnTT et de retenir le plus lisible, le plus simple à prendre en main et
  à réutiliser ensuite.

## Contexte

Le modèle objet décrit comment on représente les « choses » du jeu (entités, composants) et leur logique
(systèmes). Il faut une bibliothèque lisible, bien documentée, maintenue, et qui aide aussi pour les besoins à
venir : hiérarchies, ordonnancement des systèmes, réflexion et sérialisation pour l'éditeur.

## Options envisagées

| Critère | **flecs** | EnTT |
|---|---|---|
| Stockage | Archetypes (tables SoA) | Sparse sets (+ groupes) |
| Documentation | Site avec quickstart, manuels par sujet (requêtes, systèmes, hiérarchies, relations…), exemples, série d'articles de l'auteur sur les internes | Wiki du dépôt et commentaires du code ; série d'articles « ECS back and forth » de l'auteur |
| Prise en main | Systèmes et requêtes déclaratifs, phases d'exécution prêtes à l'emploi | API minimale : on écrit soi-même la boucle et l'ordre des systèmes |
| Hiérarchies | Natives (`ChildOf`), requêtes en cascade pour propager les transforms | À construire |
| Ordonnancement | Pipelines, phases, intervalles, tick sources, multithreading | Outil d'organisation minimal |
| Réflexion et sérialisation | Addon meta + sérialiseur JSON intégrés | Module `meta` pour la réflexion ; pas de JSON intégré |
| Outils | Explorer web qui affiche entités et composants en direct | Aucun outil visuel intégré |
| Langage | Cœur en C99, API C++17 | C++17, header-only |
| Utilisé par | Tempest Rising et plus d'une dizaine de jeux commerciaux | Minecraft (Mojang), ArcGIS Runtime (Esri) |
| Licence et maintenance | MIT, activement maintenu, port vcpkg (4.1.x) | MIT, activement maintenu, port vcpkg |

## Décision

**flecs.** Pour notre objectif (un jeu, un moteur qu'on comprend et qu'on réutilise), ses avantages décisifs sont
la documentation, l'explorer, les hiérarchies et les pipelines prêts à l'emploi, et la réflexion + JSON dont
l'éditeur aura besoin en phase 7.

Les deux bibliothèques sont bien maintenues : ce critère ne les départage pas.

## Conséquences

- La phase 3 est allégée : on intègre flecs au lieu d'écrire un ECS. Les hiérarchies viennent de `ChildOf`, les
  phases d'exécution des pipelines flecs.
- La phase 7 est allégée : réflexion et sérialisation JSON s'appuient sur l'addon meta de flecs.
- flecs est l'API du modèle objet : ses types sont visibles dans `scene/` et dans tout ce qui est au-dessus
  (physique, audio, renderer, éditeur, jeux), mais pas dans `core/`, `platform/` ni `gpu/`.
- Le code interne de flecs est en C : le lire est plus difficile que l'API C++. Pour comprendre les internes, les
  articles de Sander Mertens sont plus efficaces que le code (voir `docs/LECTURES.md`).
- Le fonctionnement des archetypes est expliqué dans l'étude E3 (modèles objets), comparé aux sparse sets d'EnTT.

## Ce que font les autres moteurs

Unreal : Actors et Components, plus Mass (ECS à archetypes) pour les foules. Unity : GameObject et
MonoBehaviour, plus DOTS/Entities (archetypes). Godot : arbre de Nodes. Bevy : ECS à archetypes.
