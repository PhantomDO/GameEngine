# E4 — Pipelines d'assets

> Écrite le 25/09/2026, en fin de M4.4. Lecture : 10 à 15 minutes.
> Convention : ce qui est **documenté** renvoie à une source ; ce qui est **déduit** est signalé.

## La question

Entre le fichier que produit un artiste (un PNG, un glTF) et ce que lit le GPU, tout moteur répond aux mêmes
cinq questions :

1. **L'identité** : comment une scène désigne-t-elle un asset, sans casser quand on le renomme ?
2. **La version dérivée** : où vit la forme prête à charger (compressée, en binaire), et qui la produit ?
3. **L'invalidation** : qu'est-ce qui rend cette version périmée ?
4. **Le rechargement** : quand le moteur s'aperçoit-il qu'une source a changé ?
5. **Le jeu livré** : qu'est-ce qui part chez le joueur ?

M4.2 à M4.4 y ont répondu pour Levain. Cette étude regarde comment Unreal, Unity et Godot y répondent, pour voir
ce qui nous manque avant *Rando*.

## 1. Ce que fait notre moteur (et pourquoi)

| Question | Levain |
|---|---|
| Identité | Un GUID dans un `.meta` voisin, rattaché par le hash si le fichier est renommé sans lui ([ADR-0019](../adr/0019-identifiants-d-assets.md)) |
| Version dérivée | `<racine>/.cooked/`, écrit par `levain_cook`, sans GPU : maître UASTC et cache BC7 ([ADR-0020](../adr/0020-cuisson-des-assets.md)) |
| Invalidation | Le hash de la source et la version du cuiseur, écrits dans l'en-tête du fichier cuit |
| Rechargement | Les dates des fichiers du registre, toutes les 100 ms ; la source est relue ([ADR-0021](../adr/0021-hot-reload-des-textures.md)) |
| Jeu livré | Pas encore décidé : aujourd'hui, `.cooked/` sert au développement et servirait tel quel |

Le principe commun : **la source fait foi, tout le reste se recalcule**. Un fichier cuit périmé n'est pas une
erreur : on repart de la source, et le log le dit.

## 2. Unreal

- **Un asset est un fichier `.uasset`**, en général un asset par fichier (documenté [1]). L'import d'un PNG ou
  d'un FBX produit un `.uasset` ; les scènes désignent les assets par leur **chemin**, et un renommage dans
  l'éditeur laisse un redirecteur (voir [ADR-0019](../adr/0019-identifiants-d-assets.md)).
- **Le *Derived Data Cache*** (documenté [2]) garde ce qui se calcule à partir des `.uasset` : shaders compilés,
  textures compressées pour la plateforme de l'éditeur. Epic le dit jetable : « Content stored in the DDC is
  disposable ». Il n'est **pas versionné**. Un **DDC partagé**, sur le réseau, évite que chaque membre de l'équipe
  recalcule les mêmes données : une seule personne à la fois doit les construire.
- **La cuisson** (documenté [3]) convertit le contenu vers le format de chaque plateforme, dans
  `Saved/Sandboxes/Cooked-<Plateforme>`. Deux modes : tout d'avance (*by the book*), ou **à la volée**, par un
  cuiseur en mode serveur auquel le jeu se connecte. Un jeu cuit n'utilise pas le DDC [2].
- **Le rechargement** : l'*Auto Reimport* surveille des dossiers choisis et réimporte un fichier source modifié
  (documenté [4]).
- **Déduit** (d'expérience, non documenté ici) : le `.uasset` d'une texture embarque la source importée. Le
  projet s'ouvre donc sans les fichiers d'origine, et la réimportation les recherche à leur ancien chemin.

## 3. Unity

- **L'identité** : un GUID dans le `.meta` de chaque asset, avec ses réglages d'import (documenté,
  [ADR-0019](../adr/0019-identifiants-d-assets.md)).
- **La version dérivée** vit dans `Library/`, non versionné. Deux bases la décrivent (documenté [5]) :
  `SourceAssetDB` (date, hash et GUID de chaque source, pour savoir s'il faut réimporter) et `ArtifactDB` (les
  **artefacts** produits par l'import, avec leurs dépendances). Unity les traite en cache, parce qu'il peut
  toujours les régénérer « using the import settings and project settings » [5].
- **L'invalidation** : un artefact dépend de la source, mais aussi des **réglages d'import** et des **réglages du
  projet**. La plateforme entre dans le hash sous lequel sont rangés les résultats : changer de plateforme
  réimporte les textures, qui n'ont pas le même format partout [5].
- **Le partage** : *Unity Accelerator* garde les résultats d'import d'un membre de l'équipe, et les sert aux
  autres (documenté [6]). C'est le DDC partagé d'Unreal.
- **Le rechargement** : au retour du focus dans l'éditeur, si l'*Auto Refresh* est actif (documenté,
  [ADR-0021](../adr/0021-hot-reload-des-textures.md)).

## 4. Godot

- **L'identité** : un fichier `<asset>.import` voisin, à versionner, porte les réglages d'import (documenté [7]).
  Depuis la 4.4, un UID permet de déplacer les fichiers hors de l'éditeur
  ([ADR-0019](../adr/0019-identifiants-d-assets.md)).
- **La version dérivée** vit dans `.godot/imported/` (des `.ctex` pour les textures, avec un `.md5`), **non
  versionné** : le versionner accélérerait un premier import, mais coûterait trop de place [7].
- **L'invalidation et le rechargement** : quand le MD5 d'une source change, Godot la réimporte, avec les réglages
  de son `.import` [7].
- **Le jeu livré** : un fichier peut être marqué « Keep File » (exporté tel quel) ou « Skip File » [7]. **Déduit** :
  par défaut, c'est donc la version importée qui part dans le paquet, pas la source.

## 5. Côte à côte

| | Unreal | Unity | Godot | Levain |
|---|---|---|---|---|
| Désignation | Chemin, redirecteurs | GUID (`.meta`) | Chemin, et UID (4.4) | GUID (`.meta`), rattaché par le hash |
| Réglages d'import | Dans le `.uasset` | Dans le `.meta` | Dans le `.import` | Aucun pour l'instant |
| Cache dérivé | DDC, local et partagé | `Library/`, et *Accelerator* | `.godot/imported/` | `.cooked/` |
| Clé du cache | (non documentée ici) | Source, réglages d'import et du projet | MD5 de la source, réglages | Hash de la source, version du cuiseur, format dans le nom |
| Détection | Dossiers surveillés | Retour du focus | MD5 | Dates, puis hash |
| Jeu livré | Cuisson par plateforme, sans DDC | Build (non étudié ici) | Paquet de fichiers importés (déduit) | À décider |

## 6. Ce qu'on en retient

1. **Les trois moteurs séparent trois couches** : la source, versionnée ; un cache dérivé, jetable, jamais
   versionné ; et ce qui part chez le joueur. **Nous n'en avons que deux** : `.cooked/` sert à la fois de cache
   et de contenu livrable. Cela suffit tant que *Rando* ne vise qu'une plateforme. La séparation viendra avec le
   paquet du jeu (phase 8), ou avec une deuxième plateforme (la Switch 2 ou le mobile de l'ADR-0020) : le cache
   BC7 et un cache ASTC ne partiront pas dans le même paquet (déduit).
2. **Notre clé de cache est incomplète, et ça se verra au premier réglage d'import.** Unity invalide un artefact
   quand ses réglages changent. Le jour où un `.meta` portera un réglage (la taille maximale d'une texture, le
   mode de compression), il faudra l'ajouter à l'en-tête du fichier cuit. Sinon, changer un réglage ne recuira
   rien. **À noter pour le premier réglage d'import**, sans rien changer aujourd'hui.
3. **Le partage du cache** (DDC partagé, *Accelerator*) a chez nous un équivalent à portée de main : le cache de
   la CI. Aujourd'hui, le job Release recuit Sponza à chaque PR (3 min 30), parce que `.cooked/` est exclu du
   cache. Le garder, sous une clé qui inclut les sources du cuiseur, serait notre *Accelerator* à l'échelle d'un
   dépôt. C'est déjà dans les pistes, à faire si l'attente de la CI gêne.
4. **Le rattachement par le hash reste notre différence** : aucun des trois ne retrouve un fichier renommé hors
   de l'éditeur sans son fichier voisin. Et Unreal, qui désigne par chemin, ne le prévoit pas du tout.
5. **Le hot-reload suit la même logique partout** : on détecte le changement de la source, puis le pipeline
   habituel refait le travail. Les autres moteurs réimportent avant d'afficher ; nous affichons la source d'abord
   (ADR-0021). Chez eux, c'est l'éditeur qui attend l'import ; chez nous, c'est le jeu qui tourne, et une image
   figée s'y verrait davantage (déduit).

## Sources

1. Epic Games, *Working with Assets in Unreal Engine* —
   https://dev.epicgames.com/documentation/en-us/unreal-engine/working-with-assets-in-unreal-engine
2. Epic Games, *Using Derived Data Cache in Unreal Engine* —
   https://dev.epicgames.com/documentation/unreal-engine/using-derived-data-cache-in-unreal-engine
3. Epic Games, *Cooking Content in Unreal Engine* —
   https://dev.epicgames.com/documentation/en-us/unreal-engine/cooking-content-in-unreal-engine
4. Epic Games, *Reimporting Assets Automatically in Unreal Engine* —
   https://dev.epicgames.com/documentation/unreal-engine/reimporting-assets-automatically-in-unreal-engine
5. Unity, *Contents of the Asset Database* — https://docs.unity3d.com/Manual/asset-database-contents.html
6. Unity, *Introduction to Unity Accelerator* —
   https://docs.unity3d.com/6000.5/Documentation/Manual/UnityAccelerator.html
7. Godot, *Import process* — https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/import_process.html
