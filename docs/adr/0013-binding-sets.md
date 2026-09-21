# ADR-0013 — Liaison des ressources : binding sets

- **Statut** : accepté le 2026-09-21 (choisi par Donnovan à la lecture du code des deux options, issue #40)
- **Date** : 2026-09-21
- **Milestone** : M2.1

## Contexte

Avant d'écrire le renderer de la phase 2, il faut décider comment les shaders trouvent leurs ressources
(textures, buffers de constantes, samplers). NVRHI propose deux modèles (étude E1, §4) :

- les **binding sets** : un *binding layout* déclare les slots HLSL (`t`, `b`, `s`, `u`) qu'attend le shader, un
  *binding set* y place les ressources réelles. Le set est immuable ; NVRHI garde ses ressources vivantes et place
  leurs barrières ;
- le **bindless** : une *descriptor table*, grand tableau modifiable que le shader indexe (par exemple par numéro de
  matériau). NVRHI n'y suit **ni la durée de vie, ni les états** des ressources.

Le choix doit tenir jusqu'à la phase 5 (PBR, ombres en cascades, éclairage d'environnement), pas seulement pour la
phase 2.

## Options envisagées

| Option | Pour | Contre |
|---|---|---|
| **Binding sets** | NVRHI garde durée de vie et barrières ; suffit à la phase 5 (un set par matériau, un par passe) ; c'est ce que fait le renderer de Donut | Un changement de set par matériau dessiné ; pas prêt pour le ray tracing |
| Bindless (descriptor tables) | Un entier par draw ; exigé par le ray tracing et le rendu piloté par le GPU | Durée de vie et états à notre charge, sans filet : une texture libérée trop tôt se lit comme de la mémoire corrompue |
| Hybride (binding sets pour les constantes, bindless pour les textures) | Le modèle des gros moteurs | Le plus de code dès M2.1, pour un besoin (ray tracing) hors de la v1 |

Le code des trois options a été présenté côte à côte, avec ses shaders Slang, avant le choix (même méthode que les
ADR-0009 et 0011).

## Décision

**Binding sets**, rangés par **fréquence de changement**, un binding layout par fréquence :

| Espace de registres | Contenu | Change |
|---|---|---|
| `space0` | constantes de frame : caméra (volatile constant buffer) | une fois par frame |
| `space1` | ressources de passe : cartes d'ombres, cubemaps d'éclairage | une fois par passe |
| `space2` | ressources de matériau : textures du PBR, sampler | à chaque matériau |

Les layouts d'un même pipeline sont créés avec `setRegisterSpaceAndDescriptorSet(n)` : sous Vulkan, l'espace `n`
devient le descriptor set `n` (NVRHI, commentaire de `BindingLayoutDesc::registerSpaceIsDescriptorSet`). Les
décalages de binding de `slangc` s'appliquent à tous les espaces (`-fvk-*-shift … all`, ADR-0005). Cette
convention est à vérifier par la validation dès les premières constantes de M2.1.

**Passer au bindless demandera un nouvel ADR**, qui remplace celui-ci, validé par Donnovan et justifié par une
mesure. Signaux qui l'ouvriraient : le ray tracing (v2) ; le rendu piloté par le GPU ; ou un coût de changement de
binding set mesuré au-dessus du budget d'une frame, avec des milliers de matériaux distincts.

## Conséquences

- La sécurité reste chez NVRHI : aucune durée de vie ni aucun état à suivre à la main dans le renderer.
- Un matériau porte son binding set, créé une fois à son chargement ; le renderer ne fabrique aucun set par frame.
- Les shaders déclarent explicitement leur espace (`register(t0, space2)`), ce qui dit à la lecture à quelle
  fréquence change chaque ressource.

## Ce que font les autres moteurs

- **Godot 4** : `RenderingDevice` n'a que des *uniform sets*, l'équivalent des binding sets, rangés eux aussi par
  fréquence (**documenté** : `rendering_device.h`, dépôt public).
- **Donut** : son renderer forward crée un binding set par matériau ; le bindless n'apparaît que dans ses
  exemples de ray tracing et *Bindless Rendering* de Donut-Samples (**documenté** : dépôts publics).
- **Unreal** : liaison par *uniform buffers* et paramètres de shader, avec un mode bindless ajouté en option dans
  ses RHI Direct3D 12 et Vulkan récentes (**documenté** par Epic ; la version exacte n'est pas vérifiée ici).
- **Unity** : ne l'expose pas aux pipelines scriptables (**non documenté**, supposé).
