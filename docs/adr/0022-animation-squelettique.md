# ADR-0022 — Animation squelettique : ozz-animation, une passerelle glTF, le skinning en compute

- **Statut** : accepté le 2026-09-25 (validé par Donnovan)
- **Date** : 2026-09-25
- **Milestone** : M4.5

## Contexte

*Rando* a un personnage animé : repos, marche, course, saut, chute, nage, vol plané (`docs/JEU.md`, M3.5). Son
modèle vient de l'*Universal Animation Library* de Quaternius (CC0), un glTF avec un squelette (*skin*) et des
clips. Le critère de M4.5 : **le personnage passe du repos à la course selon sa vitesse, sans saut visible**.

Trois choix durables précèdent le code (#116) : qui calcule les poses, où les sommets sont déformés, et comment
le monde flecs voit les os. Donnovan les a tranchés sur sondage le 25/09/2026.

Rappel des termes. Un **squelette** est une hiérarchie d'os (*joints*). Un **clip** donne, pour chaque os, des
clés de translation, de rotation et d'échelle dans le temps. **Échantillonner** un clip à un instant donne une
**pose** : la transformation locale de chaque os. Un **fondu** mélange deux poses. Le **skinning** déforme
ensuite chaque sommet du mesh selon les os qui l'influencent (quatre au plus en glTF, avec leurs poids).

## Options envisagées

**1. L'échantillonnage et le fondu**

| Option | Pour | Contre |
|---|---|---|
| **ozz-animation, avec notre passerelle depuis glTF** (proposé par Donnovan) | Bibliothèque éprouvée (MIT) : échantillonnage en SoA, fondus, IK (les pieds sur une pente), compression des clips. La passerelle lit le glTF avec fastgltf, déjà là, et remplit les structures d'import d'ozz : pas besoin de `gltf2ozz` et de sa copie de tinygltf | Une dépendance sans port vcpkg : `FetchContent` à commit figé. Des types propres à ozz, à garder derrière les nôtres. Un seul mainteneur (1 600 commits sur 1 650), actif : 0.17.0 le 01/08/2026 |
| Code maison sur fastgltf | Aucune dépendance, environ 300 lignes, le fonctionnement visible de bout en bout | Ni IK, ni compression : à écrire le jour où il en faudra |
| Maison, ozz plus tard | Commence petit | Deux intégrations au lieu d'une, si l'IK arrive |

**2. Le skinning**

| Option | Pour | Contre |
|---|---|---|
| **Un compute shader, avant le rendu** | Les sommets déformés sont écrits une fois dans un buffer, et toutes les passes les relisent comme un mesh ordinaire : les ombres (M5.3) ne refont pas le calcul. La passe des meshes ne change pas. C'est le *Skin Cache* d'Unreal | Une passe et un buffer de sortie par personnage |
| Dans le vertex shader | Le plus simple : quatre os et quatre poids par sommet, les matrices dans un buffer | Chaque passe refait le calcul, et la passe des meshes a besoin d'une variante skinnée |

**3. Les os dans le monde flecs**

| Option | Pour | Contre |
|---|---|---|
| **Une pose dans un composant** | Le personnage est une entité qui porte ses matrices d'os : rien ne déménage entre les tables flecs, et le calcul se fait d'un bloc. Un objet s'accroche à un os par un composant d'attache (le planeur dans le dos), comme les *sockets* d'Unreal ou le `BoneAttachment3D` de Godot | Les os ne se voient pas un par un dans l'explorer |
| Une entité par os | Comme les `Transform` d'Unity : chaque os se voit et s'accroche comme un parent ordinaire | 50 à 70 entités par personnage, et une hiérarchie recalculée à chaque image |

## Décision

**Un nouveau module, `engine/animation`**, au-dessus d'`assets` (SPECS §7). `render` n'en dépend pas : le
skinning ne reçoit que des matrices. Le module contient :

- **la passerelle** : fastgltf lit le squelette et les clips d'un glTF, et remplit les structures d'import
  d'ozz (`RawSkeleton`, `RawAnimation`), que ses *builders* convertissent au format d'exécution ;
- **la cuisson** : `levain_cook` écrit le squelette et les clips au format binaire d'ozz dans `.cooked/`, sous le
  GUID du glTF (ADR-0020). Sans version cuite, la passerelle tourne au chargement, avec un avertissement, comme
  pour les meshes et les textures ;
- **l'échantillonnage et les fondus**, par les *jobs* d'ozz, dans des fonctions libres (ADR-0011). Ils écrivent la
  pose dans un composant `Pose`, en matrices glm. Aucun type d'ozz ne sort du module ;
- **l'attache à un os**, par un composant qui désigne l'entité du personnage et l'indice de l'os.

**ozz-animation 0.17.0** (MIT) arrive par `FetchContent`, figé sur son commit (`83b35f1`), sans ses outils, ses
exemples ni ses tests. **ozz n'est visible que dans `engine/animation/src`**. fastgltf le devient aussi, pour
la passerelle. Le contrôle `deps.asset-libraries-visibility` s'étend aux deux.

**Le skinning se fait en compute**, dans `engine/render`. Pour chaque personnage, une passe lit les sommets
d'origine (avec leurs quatre os et leurs poids), les matrices de la pose, et écrit des sommets au format de la
passe des meshes. Cette passe les dessine ensuite sans savoir qu'ils ont été animés.

**La machine à états** (#118) reste du code C++, dans des fonctions libres : sept états et leurs transitions,
choisis par la vitesse et l'état physique du personnage. Un éditeur de graphes attendra la phase 7, s'il sert.

## Conséquences

- **Aucune autre bibliothèque ne couvre le même terrain** (recherche du 25/09/2026, [QA](../QA.md)) : ACL, le
  codec d'Unreal depuis la 5.3, ne fait que compresser les clips. Si ozz s'arrêtait, notre copie figée (MIT)
  resterait utilisable.
- **Une dépendance de plus**, `ozz-animation`, hors vcpkg : *Rando* la reçoit avec le moteur, par le même
  `FetchContent`, sans rien recopier dans son manifeste (ADR-0018).
- **ozz demande CMake 3.30** : la machine de référence a la 4.4 ; la CI est à vérifier par la PR d'intégration.
  Si elle ne l'a pas, le minimum du projet passe de 3.28 à 3.30.
- **Les nœuds glTF des os ne deviennent plus des entités** : `instantiateModel` les saute, et la pose les remplace.
- **Le skinning en compute est une première pour le moteur** : première passe compute sous NVRHI, avec ses
  barrières (NVRHI les place d'après l'état déclaré des ressources). Elle servira de modèle au culling (M5.5).
- **L'IK et la compression des clips** sont disponibles sans rien écrire, le jour où *Rando* en a besoin (les pieds
  sur la pente de la vallée, par exemple).
- **Le personnage Quaternius est un téléchargement** (CC0), à ajouter à `tools/assets.lock` par #117, avec
  l'accord de Donnovan comme pour Sponza.
- **L'implémentation dépasse 400 lignes** : elle se découpe en PR (le module et la passerelle, puis le compute et
  le rendu d'un clip, puis les fondus et la machine à états).

## Ce que font les autres moteurs

- **Unreal** (documenté [1]) : le *Skin Cache* skinne positions et normales dans un compute shader, et garde le
  résultat dans des vertex buffers que le rendu relit. Il s'active dans les réglages du projet (*Support Compute
  Skin Cache*). Un objet s'accroche à un os par un *socket* (d'expérience, non documenté ici).
- **Unity** (documenté [2]) : les os d'un `SkinnedMeshRenderer` sont un tableau de `Transform`, donc des
  GameObjects. Avec l'optimisation des GameObjects de l'avatar, ce tableau est vide : les os ne sont plus des
  objets, ce qui revient à notre choix.
- **Godot** (documenté [3]) : un nœud `Skeleton3D` porte les os, et `BoneAttachment3D` copie ou impose la
  transformation de l'un d'eux à un nœud.
- **ozz-animation** (documenté [4]) : se dit « game-engine agnostic » ; il fournit le chargement,
  l'échantillonnage et le fondu, et laisse le skinning au moteur.

## Sources

1. Epic Games, *Skeletal Mesh Rendering Paths in Unreal Engine* —
   https://dev.epicgames.com/documentation/en-us/unreal-engine/skeletal-mesh-rendering-paths-in-unreal-engine
2. Unity, *SkinnedMeshRenderer.bones* — https://docs.unity3d.com/ScriptReference/SkinnedMeshRenderer-bones.html
3. Godot, *BoneAttachment3D* — https://docs.godotengine.org/en/stable/classes/class_boneattachment3d.html
4. Guillaume Blanc, *ozz-animation* — https://github.com/guillaumeblanc/ozz-animation
