# ADR-0016 — Boucle à pas fixe, interpolation et spirale de la mort

- **Statut** : accepté le 2026-09-22 (choisi par Donnovan sur sondage, M3.3)
- **Date** : 2026-09-22
- **Milestone** : M3.3

## Contexte

Aujourd'hui, le sandbox appelle `world.progress()` une fois par image, avec le temps écoulé : la simulation
avance donc au rythme du rendu. Deux machines, ou deux moments, ne donnent pas le même résultat — et M3.3
demande justement l'inverse (issue #65) : **un état de simulation identique au bit près après N pas, que le
rendu tourne à 30, 60 ou 144 images/s**.

Trois questions à trancher ensemble, parce que leurs réponses se tiennent :

1. **qui décide du pas de la simulation**, et comment il rattrape le temps quand une image traîne ;
2. **ce que voit le joueur** entre deux pas, si le rendu va plus vite que la simulation ;
3. **ce que fait la boucle quand une image dépasse le budget**, pour éviter la « spirale de la mort » : une
   image lente demande plus de pas, qui la rendent plus lente encore.

## Options envisagées

Mesures sur la machine de référence (SPECS §10, Release).

| Option | Pour | Contre |
|---|---|---|
| **Accumulateur et pipeline flecs dédié** (la ROADMAP) | Le pas de simulation ne dépend plus de l'image : c'est la seule option qui tient le critère. Un pipeline à part n'exécute que les systèmes de simulation, N fois, sans rejouer le rendu | Il faut écrire l'accumulateur, son plafond, et garder l'état précédent pour interpoler |
| *Tick sources* de flecs (`interval(1/60)` sur les systèmes) | Rien à écrire : flecs sait déclencher un système à intervalle | **Aucun rattrapage** : mesuré, un système à 1/60 s exécuté sur 60 images à 30 images/s tourne **60 fois au lieu de 120**. La simulation suit le rendu, le critère est raté |
| Tout à pas fixe, rendu compris (`set_target_fps(60)`) | La plus simple : une seule cadence, aucune interpolation à faire | Le rendu n'utilise plus l'écran : 60 images/s sur un écran à 144 Hz, et une image lente décale toute la simulation |

Et pour ce que voit le joueur entre deux pas, l'interpolation coûte, sur 100 000 entités interpolées :
**+0,51 ms par pas** (copier l'état précédent) et **+1,47 ms par image** (l'interpolation elle-même), soit
~20 ns par entité et par image. Sur les quelques centaines d'acteurs réellement en mouvement d'un monde
ouvert, c'est 4 µs.

## Décision

**Accumulateur, pipeline de simulation dédié, interpolation automatique de ce que la simulation déplace.**

1. **Pas fixe de 60 Hz.** Le temps de l'image s'ajoute à un accumulateur ; tant qu'il contient un pas entier,
   le pipeline de simulation est exécuté avec **exactement** `1/60 s` comme `delta_time`, jamais le temps réel.
2. **Un pipeline flecs à part** (`world.pipeline().with(flecs::System).with<Simulation>()`), exécuté par
   `run_pipeline`. Le pipeline par défaut reste celui du rendu : il tourne une fois par image, et c'est lui qui
   compose les matrices monde.
3. **Interpolation automatique.** Ce que la simulation déplace porte son état précédent, copié au début de
   chaque pas ; le reste ne paie rien. Concrètement, le trait `With` de flecs attache `PreviousTransform` à
   toute entité qui a une `Velocity` — et demain, à tout corps physique. Le système des matrices monde
   compose la position affichée entre l'état précédent et l'actuel, avec le reste de l'accumulateur comme
   facteur (`alpha`). Une entité sans état précédent est rendue telle quelle, sans surcoût.
4. **Plafond de 4 pas par image.** Au-delà (une image de plus de 66 ms), le temps en trop est **abandonné** :
   la simulation ralentit par rapport à l'horloge, mais la boucle garde la main et l'image suivante repart
   propre. Sans ce plafond, une image lente demande plus de pas, qui la rendent plus lente encore.

## Conséquences

- **Un système de gameplay va dans le pipeline de simulation**, jamais dans le pipeline par défaut : son
  `delta_time` est alors toujours `1/60 s`, ce qui rend son résultat reproductible. `ApplyVelocity` y passe.
- **Le rendu ne lit plus l'état simulé brut** mais son interpolation. La conséquence visible : ce qui est
  affiché a jusqu'à un pas (16 ms) de retard sur la simulation. C'est le prix admis par tous les moteurs qui
  interpolent, et c'est invisible à côté d'une image qui saccade.
- **La rotation s'interpole en `nlerp`** (une interpolation linéaire renormalisée), pas en `slerp` : sur un
  seizième de seconde, l'écart est sous le millième de degré, et la `slerp` coûte deux fonctions
  trigonométriques par entité.
- **Le déterminisme promis est celui d'un même binaire sur une même machine** : même suite de pas, mêmes
  résultats. Le déterminisme entre machines (jeu en réseau, rejeu de parties) demanderait bien plus, et ne
  fait pas partie de la v1.
- **Le jeu ralentit plutôt que de sauter** quand la machine ne suit pas. Un joueur sur une machine trop lente
  verra le monde avancer au ralenti : c'est le choix le plus sûr pour une simulation physique, qui explose si
  on lui donne de trop grands pas.
- **L'input sera lu par image et appliqué au pas suivant** (M3.4) : une action ne peut pas se produire « entre
  deux pas ».
- **À revoir** si la simulation devient assez lourde pour que 4 pas de rattrapage coûtent trop cher, ou si un
  mode réseau demande un déterminisme entre machines.

## Ce que font les autres moteurs

- **Unity** : `FixedUpdate` tourne à `Time.fixedDeltaTime` (0,02 s par défaut), plusieurs fois par image si
  besoin, et `Time.maximumDeltaTime` **borne le rattrapage** — la documentation dit qu'il « borne le nombre de
  fois que Unity exécute `FixedUpdate` dans une image à `maximumDeltaTime / fixedDeltaTime` » (**documenté**).
  L'interpolation est un réglage **par Rigidbody**, désactivé par défaut (**documenté**).
- **Godot 4** : la simulation tourne dans `_physics_process`, à 60 Hz par défaut. L'interpolation de la
  physique est « optionnelle et désactivée par défaut », et repose sur la position du pas précédent et celle du
  pas courant (**documenté** : manuel, *Physics interpolation introduction*).
- **Unreal** : la physique peut être découpée en sous-pas de durée fixe (*substepping*), activé par projet ;
  le reste du jeu tourne à pas variable (**documenté** : documentation d'Epic sur le substepping).
- **Le patron lui-même** vient de *Fix Your Timestep!* de Glenn Fiedler : accumulateur, pas fixe, et le reste
  de l'accumulateur comme facteur d'interpolation.
- **Pour la cible visée** (un monde ouvert à la *Breath of the Wild* ou *Xenoblade*), c'est le seul modèle
  tenable : la physique y est au cœur du gameplay et tourne à pas fixe, pendant que le rendu suit l'écran
  (**documenté** pour BotW : GDC 2017, *Breaking Conventions with The Legend of Zelda: Breath of the Wild*).
  Ces mondes contiennent des milliers d'entités mais n'en déplacent que quelques centaines à la fois : d'où
  l'interpolation attachée à ce qui bouge, et non à tout ce qui existe.

## Sources

1. flecs, *Systems Manual*, sections « Custom pipeline » et « Timers » —
   https://www.flecs.dev/flecs/md_docs_2Systems.html
2. Unity, `Time.maximumDeltaTime` — https://docs.unity3d.com/ScriptReference/Time-maximumDeltaTime.html
3. Unity, `Rigidbody.interpolation` — https://docs.unity3d.com/ScriptReference/Rigidbody-interpolation.html
4. Godot, *Physics interpolation introduction* —
   https://docs.godotengine.org/en/stable/tutorials/physics/interpolation/physics_interpolation_introduction.html
5. Epic Games, *Physics Sub-Stepping* —
   https://dev.epicgames.com/documentation/en-us/unreal-engine/physics-sub-stepping-in-unreal-engine
6. Glenn Fiedler, *Fix Your Timestep!* — https://gafferongames.com/post/fix_your_timestep/
