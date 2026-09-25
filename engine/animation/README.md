# `engine/animation`

## Rôle

Animer les squelettes : lire le squelette et les clips d'un glTF skinné, échantillonner un clip, et calculer la
pose qu'utilisera le skinning ([ADR-0022](../../docs/adr/0022-animation-squelettique.md)). Le calcul lui-même
est fait par **ozz-animation** ; ce module est la frontière entre ozz et le reste du moteur.

**État en M4.5 (#117, première PR)** : ozz arrive par un port vcpkg maison (`ports/ozz-animation`), et la
passerelle glTF (`importAnimationSet`) en lit le squelette et les clips. `samplePose` échantillonne un clip en
boucle. Le skinning en compute, la cuisson, puis les fondus et la machine à états suivront.

## Invariants

1. **Aucun type d'ozz ne sort du module.** `AnimationSet` le cache derrière un pointeur vers `OzzData`,
   défini dans `src/`. Seuls `src/` incluent ozz et fastgltf (`deps.asset-libraries-visibility`).
2. **La passerelle est la nôtre** : fastgltf lit le glTF, on remplit les structures d'import d'ozz
   (`RawSkeleton`, `RawAnimation`), et ses *builders* produisent le format d'exécution. L'outil `gltf2ozz`
   d'ozz n'est pas utilisé (il embarque sa propre copie de tinygltf).
3. **Un os est un nœud du premier skin.** Les nœuds qui ne sont pas des os (le « porteur » d'un modèle) n'entrent
   pas dans la pose : elle est dans le repère du squelette. Un nœud qui n'est pas un os, placé **entre** deux os,
   est refusé, car sa transformation serait perdue.
4. **L'ordre des os est celui d'ozz**, un parent avant ses enfants, et non celui du glTF. `jointNames` le donne.
   Les noms sont uniques : un nom vide ou en double devient `os#<nœud>`.
5. **ozz n'interpole que linéairement.** Une clé glTF `STEP` devient deux clés, la seconde juste avant la
   suivante (comme `gltf2ozz`). Une clé `CUBICSPLINE` est refusée : ni Fox ni les modèles Quaternius n'en ont.
6. **Un os qu'un clip n'anime pas garde sa pose de repos**, celle de son nœud glTF.

## Points d'entrée

| Fichier | Contenu |
|---|---|
| [`include/levain/animation/animation_set.hpp`](include/levain/animation/animation_set.hpp) | `AnimationSet` (noms des os, clips), `ClipInfo`, `importAnimationSet` |
| [`include/levain/animation/pose.hpp`](include/levain/animation/pose.hpp) | `Pose` (une matrice par os), `samplePose` |
| [`src/gltf_bridge.cpp`](src/gltf_bridge.cpp) | La passerelle glTF vers ozz |
| [`src/pose.cpp`](src/pose.cpp) | Les deux *jobs* d'ozz : l'échantillonnage, puis le passage au repère du squelette |

## Équivalents ailleurs

| Moteur | Où | Ce qu'on y trouve |
|---|---|---|
| **Unreal** | `USkeleton`, `UAnimSequence`, `FAnimInstanceProxy` | Le squelette est un asset partagé entre meshes ; l'échantillonnage produit une pose locale (`FCompactPose`), convertie ensuite en espace composant (**supposé** : d'expérience). |
| **Unity** | `Avatar`, `AnimationClip`, `Animator` | Les os sont des `Transform` (**documenté**, [ADR-0022](../../docs/adr/0022-animation-squelettique.md)), que l'Animator anime (**supposé** : d'expérience). |
| **Godot** | `Skeleton3D`, `AnimationPlayer` | Le squelette porte les os comme des données, que l'`AnimationPlayer` anime (**documenté**, ADR-0022). C'est le plus proche d'ici. |
