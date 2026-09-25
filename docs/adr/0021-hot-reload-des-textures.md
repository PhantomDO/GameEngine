# ADR-0021 — Hot-reload des textures : relire la source, sans recuire

- **Statut** : accepté le 2026-09-25 (validé par Donnovan)
- **Date** : 2026-09-25
- **Milestone** : M4.4

## Contexte

Le critère de M4.4 : **une texture modifiée dans un logiciel externe est visible en moins de 2 s**. Une source
invalide ne doit pas faire planter : un message dans le log, et l'ancienne texture reste (#93).

Depuis M4.3, une texture se charge de trois façons (ADR-0020) : le cache BC7 de la plateforme, le maître UASTC
transcodé, ou la source en repli. Un fichier cuit n'est à jour que s'il porte le hash de sa source. Quand la
source change, il faut donc choisir ce que le moteur affiche : la source, ou une version recuite. Donnovan a
tranché sur sondage le 25/09/2026, avec la portée du hot-reload.

## Options envisagées

**1. Ce que charge le moteur**, mesuré sur une texture de Sponza de 1024 × 1024 (Release, trois essais) :

| Option | Pour | Contre |
|---|---|---|
| **Relire la source** | Le repli qui existe déjà : 26 ms, sans gel de l'image, à toutes les tailles | La texture reste en RGBA8 (4 fois plus de mémoire vidéo) jusqu'au prochain `levain_cook`, et le log le signale |
| Recuire, puis charger le cuit | Le fichier cuit reste à jour ; même principe que les shaders (ADR-0014) | `levain_cook` en sous-processus : **0,73 s** de gel pour du 1024². Pour du 2048², on estime environ 3 s : le critère des 2 s est manqué |
| La source, puis une recuisson en fond | Pas de gel, et le cuit remplace la source quand il est prêt | Un thread et un échange de texture en plus, environ 100 lignes |

**2. La portée**

| Option | Pour | Contre |
|---|---|---|
| **Les textures seulement** | Ce que demande le critère | Modifier un glTF demande de relancer le sandbox |
| Textures et modèles glTF | Les meshes et les matériaux suivent Blender | Environ 150 lignes de plus ; un nœud ajouté ou déplacé ne suit pas sans réinstancier les entités |

## Décision

**Le moteur relit la source d'une texture modifiée**, et seulement des textures.

- **La détection** : les dates de modification des fichiers du registre (ADR-0019), relues toutes les 100 ms,
  comme les shaders (ADR-0014). Le registre sait déjà quels fichiers surveiller, sous-dossiers compris.
- **Le rechargement** : le fichier modifié est rehaché, et son hash mis à jour dans le registre et dans son
  `.meta`. Son fichier cuit ne porte plus ce hash : le chargement habituel (`loadTextureData`) le voit périmé et
  prend la source, sans chemin de code propre au hot-reload.
- **L'échec** : une image qui ne se décode pas (un fichier invalide, ou à moitié écrit par l'éditeur) laisse la
  texture en place, avec une erreur dans le log (ADR-0008). L'écriture suivante change la date, et la texture
  est retentée.

La surveillance vit dans `engine/assets`. Le remplacement de la texture GPU, et des binding sets qui la
désignent, vit dans le sandbox, avec le cache GPU (ADR-0019).

## Conséquences

- **Le délai attendu** : au plus 100 ms de surveillance, plus la lecture de la source (26 ms pour du 1024²). Il
  reste mesuré par la PR d'implémentation.
- **Après une retouche, le moteur tourne sur la source** jusqu'au prochain `levain_cook`. L'écart visuel est
  celui de M4.3 (PSNR de 48 dB, imperceptible) ; la mémoire vidéo de la texture est multipliée par 4. Le log le
  dit à chaque rechargement.
- **Le `.meta` change** quand sa source change, puisqu'il porte son hash. Le scan de démarrage le faisait déjà
  (ADR-0019, cas 1).
- **Les modèles glTF ne se rechargent pas**, ni les images qu'ils embarquent. Ce sera à voir avec l'éditeur
  (phase 7), ou plus tôt si *Rando* le demande.
- **Évolution possible** : la recuisson en fond (la troisième option du choix 1), si travailler sur une texture non compressée gêne.
  Elle s'ajoute à ce choix sans le défaire.

## Ce que font les autres moteurs

- **Unity** (documenté [1]) : l'éditeur relit le dossier `Assets` quand il reprend le focus, si l'*Auto
  Refresh* est activé, et réimporte les fichiers modifiés.
- **Unreal** (documenté [2]) : l'*Auto Reimport* surveille des dossiers choisis, et réimporte un fichier source
  modifié dans les assets qui en dépendent.
- **Godot** (documenté [3]) : un asset dont le MD5 de la source a changé est réimporté automatiquement, avec les
  réglages de son `.import`.
- **Notre différence** (déduit) : les trois réimportent, avec les réglages de compression de l'asset, avant
  d'afficher. Nous affichons la source d'abord, et la cuisson reste un geste explicite (ADR-0020).

## Sources

1. Unity, *Refreshing the Asset Database* —
   https://docs.unity3d.com/6000.2/Documentation/Manual/AssetDatabaseRefreshing.html
2. Epic Games, *Reimporting Assets Automatically in Unreal Engine* —
   https://dev.epicgames.com/documentation/unreal-engine/reimporting-assets-automatically-in-unreal-engine
3. Godot, *Import process* — https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/import_process.html
