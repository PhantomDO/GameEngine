# E3 — Modèles objets

> Écrite le 23/09/2026, en fin de phase 3. Lecture : 15 à 20 minutes.
> Convention : ce qui est **documenté** renvoie à une source ; ce qui est **déduit** est signalé.

## La question

La phase 3 a mis un ECS au cœur du moteur. Tout moteur répond à deux questions, souvent confondues :

1. **Le stockage** : où vivent en mémoire les données d'un objet du jeu, et combien coûte de les parcourir ou
   de leur ajouter un composant ?
2. **Le modèle** : sous quelle forme le développeur manipule-t-il un objet ? Un Actor, un GameObject, un Node,
   ou une entité nue ?

Elles comptent maintenant : *Rando* va répartir son gameplay en plugins (M3.6), et l'éditeur devra montrer ces
objets (phase 7).

## 1. Ce que fait notre moteur (et pourquoi)

- **Une entité flecs est un identifiant**, ses composants sont des structures sans logique (`Transform`,
  `Velocity`), et ses systèmes sont des **fonctions libres** dont la glu ECS tient en une ligne
  ([ADR-0011](../adr/0011-retour-au-cpp.md)). Un **module** flecs (`SceneModule`) regroupe composants et
  systèmes.
- **Stockage par archetypes** : flecs range ensemble, dans une *table*, les entités qui ont exactement les
  mêmes composants.
- **La hiérarchie par le composant `flecs::Parent`**, et non par la relation `ChildOf`, qui aurait créé une table
  par parent ([ADR-0015](../adr/0015-stockage-de-la-hierarchie.md)).
- **La simulation dans un pipeline à pas fixe**, séparée du rendu ([ADR-0016](../adr/0016-boucle-a-pas-fixe.md)).

## 2. Deux façons de ranger les composants

| | **Archetypes** (tables) | **Sparse sets** |
|---|---|---|
| Rangement | Une table par combinaison de composants ; chaque composant y est un tableau contigu | Un tableau par type de composant, plus un index de l'entité vers sa case |
| Parcourir plusieurs composants | Rapide : tout est aligné dans la même table | Il faut croiser les tableaux, sauf à les réordonner (les *groups* d'EnTT) |
| Ajouter ou retirer un composant | **L'entité déménage** dans une autre table | Une case ajoutée ou retirée dans un seul tableau |
| Le risque | **La fragmentation** : beaucoup de tables presque vides | Des parcours multi-composants moins rapides |
| Qui | flecs, Unity Entities, Unreal Mass, Bevy (par défaut) | EnTT, Bevy (sur demande) |

- **Unity Entities** (documenté [1]) : un archetype regroupe les entités de même composition, rangées par blocs
  de **16 Kio** (*chunks*) où chaque tableau est « tightly packed ». Ajouter ou retirer un composant déplace
  l'entité, et la documentation prévient que ces déplacements fréquents coûtent cher.
- **EnTT** (documenté [2]) : un *pool* par type de composant, qui est un sparse set. Les *views* ne touchent pas
  au stockage ; les *groups* réordonnent les pools pour aligner les composants qu'ils possèdent, au prix d'un
  surcoût à la création et à la destruction.
- **Bevy** (documenté [3]) : les deux. Stockage en table par défaut, « optimized for query iteration » ; un
  composant marqué `SparseSet` est optimisé pour l'ajout et le retrait fréquents.
- **flecs** (documenté [4], vérifié dans `flecs.h`) : des tables, mais des *traits* changent le rangement d'un
  composant : `Sparse`, `DontFragment` (hors des tables), et `CanToggle` (activer ou désactiver sans
  déménager). Pour la hiérarchie, flecs propose les deux formes.

**La fragmentation, nous l'avons mesurée** en M3.2 : 100 000 entités en chaînes de 10, rangées par `ChildOf`,
font **180 264 tables** et coûtent **15,1 ms** ; par `Parent`, 280 tables et **1,45 ms**. La documentation de
flecs le dit aussi : parcourir 1 000 tables au lieu d'une peut coûter « over an order of magnitude » [4]. Elle
conseille `ChildOf` pour les grandes hiérarchies non structurées, et `Parent` pour beaucoup de petites
hiérarchies structurées. C'était notre cas : 10 000 petites chaînes.

## 3. Unreal (documenté)

Deux modèles cohabitent.

- **Actors et Components** [5] : un Actor est un conteneur de composants. Une *Actor Component* n'a pas de
  transform (déplacement, inventaire) ; une *Scene Component* en a une et peut s'attacher à une autre ; une
  *Primitive Component* se dessine ou entre en collision. La position de l'Actor est celle de sa *root
  component*. Chaque composant peut avoir son **Tick**, désactivé par défaut. Données et logique vivent dans le
  même objet.
- **Mass** [6] : un ECS à archetypes. Les *fragments* sont les données, les *tags* des fragments vides, les
  entités de même composition sont rangées par *chunks*. Des *processors* sans état portent la logique, et les
  changements de composition sont différés par un *command buffer*.
- **Déduit** : Mass sert quand les Actors coûtent trop cher (les foules, la circulation). Le gameplay courant
  reste en Actors.

## 4. Unity (documenté)

- **GameObjects** [7] : un GameObject est un conteneur de composants, et les scripts sont des composants. **Chaque
  GameObject a un Transform**, qui porte aussi la hiérarchie parent-enfant.
- **Entities (DOTS)** [1] : l'ECS à archetypes décrit plus haut.
- **Le pont** [8] : le *baking*, un processus de l'éditeur seul, convertit des GameObjects d'édition en entités
  optimisées pour l'exécution, sans retour possible. On édite en GameObjects, et le jeu tourne en entités.

## 5. Godot (documenté)

- **Nodes et scènes** [9] : chaque Node porte données et logique. La composition se fait **par nœuds enfants**
  (un corps rigide, son sprite, sa forme de collision), avec l'héritage pour les spécialiser. L'arbre de scène
  se lit directement dans l'éditeur.
- **Le data-oriented est dans les serveurs** [9] : physique, rendu et audio sont optimisés à part. L'article
  réserve l'ECS aux jeux de « dizaines de milliers d'objets » (city builders, sandbox), et n'interdit pas d'en
  ajouter un en extension.

## 6. Ce qu'on en retient

- **Notre stockage est celui de Unity Entities, d'Unreal Mass et de Bevy.** Notre modèle, en revanche, est celui
  de **Bevy seul** : nous n'avons ni couche Actor ni couche GameObject au-dessus. Unreal et Unity gardent un
  modèle objet pour le gameplay et réservent l'ECS aux cas massifs ; Godot garde ses nœuds. (Déduit.)
- **Ce que ça coûte, pour *Rando*** (déduit) :
  - ce que le designer appelle « une pomme » ou « un piège » n'existe pas comme type ; c'est un assemblage de
    composants. L'équivalent des Blueprints, des prefabs Unity et des scènes Godot est le **prefab flecs**
    (relation `IsA`), à examiner dès l'import glTF (M4.1) et la sérialisation (M7.3) ;
  - **l'éditeur doit montrer des entités sans type** : la réflexion par l'addon meta de flecs (M7.2) fait le
    travail que fait `UCLASS` pour Unreal.
- **Un plugin gameplay est un module flecs** (déduit, à trancher dans l'ADR de M3.6) : il déclare ses composants,
  leur réflexion et ses systèmes, et le jeu l'importe. C'est l'équivalent le plus direct d'un plugin Unreal qui
  enregistre ses classes.
- **Le piège qui nous attend : les changements d'état.** Si « nage » et « planeur » deviennent des tags ajoutés
  et retirés, chaque changement fait déménager l'entité. Unity documente ce coût, et Mass diffère ces
  changements par un *command buffer*.
  Pour un seul joueur, c'est négligeable. Pour des centaines de pièges ou de pommes qui changent d'état, les
  traits `CanToggle` et `DontFragment` de flecs, ou un champ d'état dans un composant, évitent la
  fragmentation que M3.2 a mesurée. **À mesurer avant de choisir.**

## 7. Ce que la phase 3 a mesuré

| Mesure | Valeur | Milestone |
|---|---|---|
| 100 000 entités, `Transform` + `Velocity` | 0,071 à 0,095 ms | M3.1 |
| 100 000 entités sur 10 niveaux, matrices monde | 1,45 ms (`Parent`) ; `ChildOf` : 15,1 ms, 180 264 tables | M3.2 |
| Un pas de simulation, 100 000 entités | 0,156 ms | M3.3 |
| Passe de rendu interpolée, 100 000 entités | 2,09 ms | M3.3 |
| État identique au bit près à 30, 60 et 144 images/s | oui, sans tolérance | M3.3 |

Commande : `./build/linux-release/tests/levain_scene_bench`, sur la machine de référence (SPECS §10).

Pour aller plus loin : D8 (flecs vu de l'intérieur, par son auteur), D10 (les sparse sets, par l'auteur
d'EnTT) et D13 (le point de vue de Godot), dans [LECTURES.md](../LECTURES.md).

## Sources

1. Unity, *Entities — Archetypes concepts* —
   https://docs.unity3d.com/Packages/com.unity.entities@1.3/manual/concepts-archetypes.html
2. EnTT, wiki *Entity Component System* — https://github.com/skypjack/entt/wiki/Entity-Component-System
3. Bevy, trait `Component`, section sur le stockage —
   https://docs.rs/bevy/latest/bevy/ecs/component/trait.Component.html
4. flecs, *Hierarchies Manual* — https://www.flecs.dev/flecs/HierarchiesManual.html
5. Epic Games, *Components in Unreal Engine* —
   https://dev.epicgames.com/documentation/en-us/unreal-engine/components-in-unreal-engine
6. Epic Games, *Overview of Mass Entity in Unreal Engine* —
   https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-mass-entity-in-unreal-engine
7. Unity, *GameObject* — https://docs.unity3d.com/6000.2/Documentation/Manual/class-GameObject.html
8. Unity, *Entities — Baking overview* —
   https://docs.unity3d.com/Packages/com.unity.entities@1.3/manual/baking-overview.html
9. Juan Linietsky, *Why isn't Godot an ECS-based game engine?* (2021) —
   https://godotengine.org/article/why-isnt-godot-ecs-based-game-engine/
