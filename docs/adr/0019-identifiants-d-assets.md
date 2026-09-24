# ADR-0019 — Identifier les assets : un GUID dans un `.meta`, rattaché par le hash

- **Statut** : proposé
- **Date** : 2026-09-24
- **Milestone** : M4.2

## Contexte

En M4.1, une entité importée désigne son mesh par un **indice provisoire** (`MeshInstance`) dans le modèle que
l'application vient de charger : rien ne survit à un redémarrage, et deux modèles se marchent dessus. M4.2
doit donner aux assets une identité durable. Ses critères : **renommer ou déplacer un fichier ne casse aucune
référence**, et **aucun chemin absolu dans les scènes**. L'issue #89 demande en plus de dire ce qui se passe
quand un fichier est renommé ou déplacé **hors du moteur**, dans un gestionnaire de fichiers ou par `git mv`.

Six décisions, prises par Donnovan sur sondages le 24/09/2026, toutes conformes à la recommandation.

## Options envisagées

**1. L'identité d'un asset**

| Option | Pour | Contre |
|---|---|---|
| **Un GUID dans un fichier `.meta` voisin** (Unity, Godot) | L'identité ne dépend ni du chemin ni du contenu ; les références ne contiennent jamais de chemin | Un fichier de plus par asset, à versionner avec lui |
| Le chemin, et des redirecteurs (Unreal) | Rien de plus sur le disque | Un déplacement hors du moteur casse tout : le critère de M4.2 est raté |
| Le hash du contenu | Rien de plus sur le disque | Modifier une texture change son identité, et casse ses références |

**2. Un fichier renommé hors du moteur, sans son `.meta`** : `foo.png` devient `bar.png`, et `foo.png.meta`
reste derrière. Chez Unity et Godot, la référence est cassée.

| Option | Pour | Contre |
|---|---|---|
| **Rattacher par le hash** | Le `.meta` garde aussi le hash du contenu : au scan, un `.meta` orphelin et un fichier sans `.meta` de même contenu se retrouvent, et le GUID est conservé | Un fichier renommé **et** modifié entre deux lancements se perd, mais c'est signalé |
| Comme Unity | Rien à écrire | Le critère de M4.2 repose sur la discipline de chacun |
| Renommer seulement dans le moteur | Aucun cas à deviner | Rien avant l'éditeur (phase 7) |

**3. La granularité** : un glTF contient plusieurs meshes et textures.

| Option | Pour | Contre |
|---|---|---|
| **Un asset par fichier, des sous-assets par indice** | Un `.meta` par fichier ; un mesh se désigne par (GUID, indice), comme les *sub-assets* d'Unity (`fileID`) | Réordonner les meshes dans le fichier source change leurs indices |
| Un GUID par élément, rangés dans le `.meta` du glTF | Chaque élément a son identité propre | Le `.meta` change dès que le glTF change |

**4. Le format du `.meta`** : **texte « clé = valeur »**, comme `data/input.cfg`. Il se lit dans une PR, et
n'ajoute aucune dépendance. JSON et YAML (le format d'Unity) ont été écartés pour la même raison.

**5. Le comptage de références**

| Option | Pour | Contre |
|---|---|---|
| **Compté par le monde, déchargé à zéro** | Les composants ne stockent qu'une référence ; des observateurs flecs comptent les entités qui l'utilisent, et l'asset se décharge en fin d'image quand le compte tombe à zéro | Un asset utilisé hors du monde (une interface, un outil) doit être compté à la main |
| Des handles RAII (`shared_ptr`, `TSharedPtr`) | Classique | Le compte change à chaque copie de composant, sans que cela se voie dans le code |
| Pas de déchargement en v1 | Rien à écrire : *Rando* charge sa vallée au démarrage | Retarde un problème que le streaming de la v2 posera de toute façon |

**6. Qui crée les `.meta`** : **le moteur, au démarrage**, comme les éditeurs d'Unity et de Godot. Il les
liste dans le log, et **la CI échoue si un asset versionné n'a pas son `.meta`**. Une commande explicite
(`levain_assets scan`) a été écartée : un oubli se paierait au premier lancement de quelqu'un d'autre.

## Décision

**Un asset est un fichier** d'un type importable (images et modèles glTF, pour l'instant), sous une racine
d'assets déclarée par le projet. Son identité est un **GUID de 128 bits**, écrit dans un fichier voisin
`<fichier>.meta` avec le hash de son contenu :

```
# Levain (ADR-0019) : à versionner avec le fichier.
guid = 7c1e0f5a93d24b8e8a0f6b2d4c9e1a37
hash = 1f3a5c7e9b2d4f60
```

**Une référence est un `AssetRef`** : le GUID, et l'indice d'un sous-asset (un mesh du glTF). Les composants
ne stockent qu'elle, jamais de chemin. Le chemin n'existe que dans le **registre**, construit au démarrage en
parcourant les racines d'assets.

**Le scan des racines**, dans cet ordre :

1. Un fichier avec son `.meta` : il est enregistré sous son GUID.
2. Un fichier sans `.meta`, et un `.meta` orphelin **de même hash** : c'est un renommage. Le `.meta` est
   renommé, le GUID conservé, et le rattachement journalisé.
3. Un fichier sans `.meta` et sans orphelin qui lui corresponde : c'est un nouvel asset. Un GUID est tiré, le
   `.meta` écrit et journalisé.
4. Un `.meta` qui reste orphelin : l'asset a disparu, ou il a été renommé **et** modifié. C'est un avertissement
   au scan, puis une **erreur au chargement** de toute référence vers lui (règle n°7).
5. Deux fichiers avec le même GUID (un fichier copié avec son `.meta`) : c'est une **erreur**, qui nomme les deux
   fichiers, et le scan ne choisit pas à la place de l'utilisateur.

**Le registre compte les entités** qui portent une référence, par des observateurs flecs sur l'ajout et le
retrait du composant. Un asset dont le compte tombe à zéro se décharge **en fin d'image**, jamais au milieu
d'un parcours.

## Conséquences

- **Le hash n'est pas cryptographique** : FNV-1a sur 64 bits, avec la taille du fichier. Il ne sert qu'à
  reconnaître un fichier renommé parmi les orphelins d'un même projet, pas à se protéger d'un fichier
  malveillant. Aucune dépendance n'est ajoutée.
- **Les `.meta` se versionnent** avec leurs fichiers (`data/textures/checker.png.meta`, par exemple). Ceux
  des assets téléchargés (`assets-cache/`) ne se versionnent pas, comme leurs fichiers.
- **`MeshInstance` disparaît** au profit d'une référence (`AssetRef`). Le cache GPU, dans l'application en
  attendant un module de rendu qui le prenne en charge, suit le même compte.
- **Réordonner les meshes d'un glTF** dans l'outil qui l'a produit change leurs indices : les entités
  pointeront vers un autre mesh. C'est le prix de la granularité par fichier ; Unity a le même problème avec
  ses `fileID`. Un nom de sous-asset serait plus robuste, mais les noms glTF ne sont pas uniques.
- **Les réglages d'import** (compression, taille maximale des textures) s'ajouteront au `.meta` avec la cuisson
  (M4.3), comme chez Unity. Le format « clé = valeur » l'accepte sans rien changer.
- **L'implémentation (#90) dépasse les 400 lignes** : elle se découpera en PR successives (le `.meta` et le
  scan, puis les références et le comptage, puis la migration du sandbox et le contrôle de la CI).

## Ce que font les autres moteurs

- **Unity** (documenté [1]) : un `.meta` par asset contient son identifiant unique et ses réglages d'import. Les
  données importées vont dans `Library/`. « If an asset loses its `.meta` file, any reference to that asset is
  broken. »
- **Godot 4.4** (documenté [2]) : les ressources importées avaient déjà un UID dans leur `.import`. Les scripts
  et shaders, qui n'en avaient pas, reçoivent un fichier `.uid` voisin, « to safely move files outside Godot »,
  à versionner avec eux. Les références s'écrivent `uid://…`.
- **Unreal** (documenté [3]) : les assets sont référencés par leur **chemin**. Un renommage fait dans l'éditeur
  laisse un *redirecteur* à l'ancien emplacement, que l'on « répare » ensuite en réenregistrant les références.
  La documentation ne prévoit pas le déplacement hors de l'éditeur.
- **Notre différence** (déduit) : le rattachement par le hash. Ni Unity ni Godot ne retrouvent un fichier dont
  le `.meta` ne l'a pas suivi.

## Sources

1. Unity, *Asset metadata* — https://docs.unity3d.com/6000.2/Documentation/Manual/AssetMetadata.html
2. Godot, *UID changes coming to Godot 4.4* — https://godotengine.org/article/uid-changes-coming-to-godot-4-4/
3. Epic Games, *Asset Redirectors in Unreal Engine* —
   https://dev.epicgames.com/documentation/en-us/unreal-engine/asset-redirectors-in-unreal-engine
