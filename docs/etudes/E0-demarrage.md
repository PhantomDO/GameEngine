# E0 — Comment démarre un moteur

> Écrite le 20/09/2026, à la clôture de la phase 0. Lecture : 12 à 15 minutes.
> Convention : ce qui est **documenté** renvoie à une source ; ce qui est **déduit** est signalé.

## La question

Entre le `main()` du système et la première image à l'écran, il se passe beaucoup de choses. Dans quel ordre, et
pourquoi cet ordre-là ? La question paraît triviale jusqu'au moment où on doit décider si les logs s'initialisent
avant ou après la lecture de la configuration — et où on découvre que les deux se veulent premiers.

## 1. Le problème : un graphe de dépendances qu'on ne peut pas trier naïvement

Un moteur démarre une quinzaine de sous-systèmes, et chacun a besoin des autres :

- les **logs** veulent la configuration, pour savoir quelles catégories activer ;
- la **configuration** veut le système de fichiers, pour lire son fichier ;
- le **système de fichiers** veut les logs, pour signaler un fichier illisible ;
- le **renderer** veut la fenêtre, qui veut la plateforme, qui veut l'allocateur.

Le cycle logs ↔ configuration ↔ fichiers est réel, et tous les moteurs le cassent de la même façon : **le
démarrage se fait en plusieurs passes**, une première où les briques de base n'ont que des réglages par défaut,
une seconde où on les reconfigure une fois la configuration lue. C'est l'idée centrale de cette étude, et elle
explique la forme du code de démarrage des trois moteurs.

## 2. Unreal — `FEngineLoop`, trois étapes nommées

Unreal découpe explicitement : `FEngineLoop::PreInit()`, `FEngineLoop::Init()`, puis `FEngineLoop::Tick()` à
chaque frame (**documenté** : [FEngineLoop](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Launch/FEngineLoop)).

**`PreInit`** fait tout ce qui doit exister avant que quoi que ce soit d'autre puisse tourner : analyse de la
ligne de commande, positionnement de `GIsEditor`, chargement des modules fondamentaux (`CoreUObject`, `Engine`,
`Renderer`, `RenderCore`), localisation, plugins, fichiers de configuration, système de scalabilité
(**documenté** : [PreInit](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Launch/FEngineLoop/PreInit)).

La ligne de commande est analysée **en tout premier**, avant les fichiers de configuration. C'est ce qui permet
à `-log` ou `-ini` d'agir sur le chargement de la configuration elle-même. L'ordre encode une hiérarchie :
*ligne de commande > fichier de configuration > défauts compilés*.

**`Init`** monte ce qui dépend d'un socle vivant : task graphs, pools de threads, application Slate, renderer,
puis `GEngine->Init()`, et enfin la diffusion des événements « moteur initialisé » pour que les modules tiers
s'accrochent.

**`Tick`** est la frame : synchronisation, événements de début et de fin de frame, statistiques, scrutation des
entrées, `GEngine->Tick()`, profilage GPU.

**Ce qu'on en retient** : les trois noms sont une documentation en soi. Un nouveau venu sait où poser son code
rien qu'en lisant la signature.

## 3. Unity — la boucle est une donnée, pas du code

Unity expose sa boucle comme une **structure de données modifiable**. Un `PlayerLoopSystem` représente un
sous-système mis à jour à chaque itération, et contient un `subSystemList` d'autres systèmes
(**documenté** : [PlayerLoopSystem](https://docs.unity3d.com/ScriptReference/LowLevel.PlayerLoopSystem.html)).

On récupère la boucle courante avec `PlayerLoop.GetCurrentPlayerLoop()`, on y insère, retire, réordonne ou
remplace des systèmes, puis on la réinstalle avec `PlayerLoop.SetPlayerLoop()`
(**documenté** : [Customizing the Player loop](https://docs.unity3d.com/6000.2/Documentation/Manual/player-loop-customizing.html)).
Le nouvel ordre ne prend effet qu'à l'itération complète suivante.

**C'est une différence de nature, pas de degré.** Chez Unreal, l'ordre des systèmes est écrit dans le code de
`Tick()` ; chez Unity, c'est un arbre qu'on manipule à l'exécution. Le prix est qu'un système retiré par
mégarde ne tourne simplement plus, sans erreur — la documentation le dit : « seuls les systèmes inclus dans la
nouvelle boucle s'exécutent ».

**Déduit, non documenté** : cette souplesse est vraisemblablement ce qui a permis de greffer DOTS sur la boucle
historique sans la réécrire.

## 4. Godot — `Main::setup`, `Main::start`, `Main::iteration`

Godot sépare lui aussi en passes, dans [`main/main.cpp`](https://github.com/godotengine/godot/blob/master/main/main.cpp) :
`Main::setup()` puis `Main::setup2()` pour l'initialisation, `Main::start()` pour charger la scène, et
`Main::iteration()` pour la frame (**documenté** : dépôt public).

Le point intéressant est `Main::iteration()`, qui contient **la boucle à pas fixe** : un accumulateur de temps
et `physics_ticks_per_second` déterminent combien d'itérations de physique exécuter avant le rendu
(**documenté** : [Idle and Physics Processing](https://docs.godotengine.org/en/stable/tutorials/scripting/idle_and_physics_processing.html)).
D'où les deux points d'entrée exposés aux scripts : `_process` à chaque frame rendue, `_physics_process` à
cadence fixe.

**Ce qu'on en retient** : la frame n'est pas « une itération ». C'est *zéro à plusieurs* pas de simulation,
suivis d'un rendu. Un moteur qui appelle la simulation une fois par frame a un comportement qui dépend du taux
de rafraîchissement — bug classique qu'on ne voit qu'en changeant d'écran.

## 5. Ce que fait notre moteur

En phase 0, `main()` tient en quinze lignes : bannière, une arène, une boucle simulée de 120 frames. Il n'y a
pas encore de démarrage à séquencer. Mais trois décisions sont déjà prises, et elles viennent de cette étude.

**Les logs n'ont pas besoin d'initialisation.** `levain::core::log()` crée sa catégorie à la première
utilisation. C'est ce qui casse le cycle logs ↔ configuration décrit en §1 : aucun code ne peut être « trop tôt »
pour journaliser, et il n'y a pas d'ordre à respecter. La configuration ne fera que baisser des niveaux ensuite,
avec `setLogLevel`.

**Les erreurs de démarrage sont des `Result`, pas des exceptions** (ADR-0008). Un shader introuvable au
démarrage n'est pas un bug : c'est une information à remonter jusqu'à `main`, qui décide d'abandonner ou de
continuer en mode dégradé.

**La frame sera un accumulateur, pas une itération**, comme chez Godot. C'est déjà écrit dans SPECS §7
(« simulation à pas fixe, 60 Hz, avec accumulateur ; rendu à fréquence libre avec interpolation ») et ça fera
l'objet d'un ADR en M3.3.

**Ce qu'on ne copiera pas** : la boucle-donnée d'Unity. Elle résout un problème qu'on n'a pas — laisser des
tiers s'insérer dans la boucle d'un moteur fermé. flecs nous donne déjà des *phases* de pipeline, qui couvrent
le besoin d'ordonnancement sans rendre la boucle modifiable à l'exécution.

## 6. Ce qu'on en retient

1. **Le démarrage se fait en plusieurs passes** parce que le graphe de dépendances a des cycles. Les trois
   moteurs le font, avec trois vocabulaires différents.
2. **L'ordre d'initialisation encode une hiérarchie de priorité** : Unreal lit la ligne de commande avant les
   fichiers de configuration, et ce n'est pas un détail d'implémentation.
3. **Un sous-système qui n'a pas d'initialisation ne peut pas être initialisé trop tard.** C'est le choix qu'on
   a fait pour les logs, et c'est la solution la moins chère au cycle de départ.
4. **Une frame n'est pas un pas de simulation.** Confondre les deux est le bug classique du débutant, et aucun
   des trois moteurs ne le fait.

## Sources

- [FEngineLoop](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Launch/FEngineLoop) et
  [FEngineLoop::PreInit](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Launch/FEngineLoop/PreInit) — Epic
- [PlayerLoopSystem](https://docs.unity3d.com/ScriptReference/LowLevel.PlayerLoopSystem.html),
  [PlayerLoop](https://docs.unity3d.com/ScriptReference/LowLevel.PlayerLoop.html) et
  [Customizing the Player loop](https://docs.unity3d.com/6000.2/Documentation/Manual/player-loop-customizing.html) — Unity
- [`main/main.cpp`](https://github.com/godotengine/godot/blob/master/main/main.cpp) et
  [Idle and Physics Processing](https://docs.godotengine.org/en/stable/tutorials/scripting/idle_and_physics_processing.html) — Godot
- **Pas de source publique trouvée** sur le démarrage de REEngine, Anvil ou Frostbite. Rien supposé.
