# Roadmap v1

> Version 0.4 — 20/09/2026 — statut : **validé par Donnovan**
>
> v0.2 : NVRHI remplace la RHI maison (ADR-0002) et flecs remplace l'ECS maison (ADR-0004). Le jalon « RHI
> mince » disparaît, les phases 1, 2, 3, 5 et 7 sont allégées, un backend Direct3D 12 et le choix du jeu sont
> ajoutés. Total : **77 h → 68 h**.
>
> v0.3 : passage à Rust (ADR-0010), annulé le jour même.
> v0.4 : **retour au C++23** (ADR-0011), périmètre **Linux d'abord**. **M1.4 (backend Direct3D 12) reste hors
> périmètre** — différé jusqu'à ce qu'une machine Windows soit disponible, et non plus supprimé. **M0.5 (retour
> au C++) ajouté.** Total : **68 h → 68 h** (−1,0 h pour M1.4 différé, +1,0 h pour M0.5).

## Comment lire les estimations

- **Heures Donnovan** : temps de Donnovan (pilotage de la session, réponses aux questions de Claude, relecture,
  questions). C'est la ressource rare, donc l'unité de planification.
- **Sessions Claude Code** : nombre de sessions de travail estimé. Chaque session consomme le quota de
  l'abonnement Claude.
- **Calendrier** : calculé pour **1,5 h par semaine**, à partir du lundi 21/09/2026. Les dates servent d'échéances
  aux milestones GitHub.
- **Incertitude** : les estimations de départ sont à ±50 %. À la fin de chaque phase, on compare l'estimé au
  réel, puis on recalcule la suite avec le ratio observé (voir « Recalibrage »).

## Synthèse

| Phase | Contenu | Heures Donnovan | Sessions | Fin visée (1,5 h/sem.) |
|---|---|---:|---:|---|
| 0 | Fondations | 5,0 | 4 | 11/10/2026 |
| 1 | Fenêtre et premier triangle | 4,5 | 3 | 01/11/2026 |
| 2 | 3D de base | 5,5 | 4 | 29/11/2026 |
| 3 | Scène et ECS | 6,0 | 4 | 27/12/2026 |
| 4 | Assets | 9,0 | 7 | 07/02/2027 |
| 5 | Rendu PBR | 11,0 | 8 | 04/04/2027 |
| 6 | Physique | 6,5 | 4 | 02/05/2027 |
| 7 | Éditeur | 9,5 | 7 | 13/06/2027 |
| 8 | Audio et le jeu | 11,0 | 8 | 08/08/2027 |
| **Total** | | **68,0** | **49** | |

Durée totale selon le rythme : **2 h/sem. → environ 8 mois** (mi-mai 2027) · **1,5 h/sem. → environ 11 mois**
(début août 2027) · **1 h/sem. → environ 16 mois** (janvier 2028).

Jalons visibles : **premier triangle** le 01/11/2026 · **choix du jeu** le 27/12/2026 · **le jeu jouable** le 01/08/2027.

### Pourquoi 68 h et pas 60

L'estimation annoncée à l'oral (« autour de 60 h ») ne tenait pas compte de deux ajouts : le backend Direct3D 12
(1 h) et le choix du jeu (0,5 h). Surtout, ce que NVRHI et flecs ne touchent pas (fondations, assets, input,
physique, gizmos, audio, le jeu et son bilan) pèse à lui seul 34,5 h.

---

## Phase 0 — Fondations

| Milestone | Heures D. | Sessions | Échéance |
|---|---:|---:|---|
| M0.1 Dépôt, suivi et specs | 1,0 | 0 | 27/09/2026 |
| M0.2 Squelette de build et CI | 1,5 | 2 | 04/10/2026 |
| M0.5 Retour au C++ | 1,0 | 1 | 27/09/2026 |
| M0.3 Core minimal | 1,5 | 1 | 11/10/2026 |

**M0.1 — Dépôt, suivi et specs.** Specs, roadmap et ADR validés ; dépôt GitHub public créé ; board, labels et
milestones en place (`tools/github-bootstrap.sh`) ; machine de référence renseignée.
*Critère* : toutes les issues des phases 0 et 1 existent sur le board avec une estimation.

**M0.2 — Squelette de build et CI.** Arborescence, `CMakePresets.json`, `vcpkg.json` (avec nvrhi et flecs),
exécutable `sandbox` qui affiche une ligne de log, doctest, clang-format, clang-tidy, workflow GitHub Actions
Windows + Linux, cache vcpkg, protection de `main`.
*Critères* : build depuis un clone propre avec deux commandes sur chaque OS ; CI verte sur les deux ; temps de
CI à froid et à chaud mesurés et notés.

**M0.5 — Retour au C++.** Le projet est passé à Rust puis revenu au C++ le 20/09/2026 (ADR-0010 puis
ADR-0011). Le retour apporte trois changements : périmètre **Linux d'abord**, presets et CI Windows retirés, et
la **forme du code** fixée par l'ADR-0011 — fonctions libres, dépendances dans la signature, pièges nommés.
*Critères* : build et tests verts sur Linux ; presets et CI Windows retirés ; ADR-0011 accepté.

**M0.3 — Core minimal.** Logs, assertions, politique de gestion d'erreurs (ADR-0008), horloge haute
résolution, lecture de fichiers, allocateurs linéaire (par frame) et pool, intégration de Tracy.
*Critères* : tests unitaires des allocateurs verts ; benchmark allocateurs vs `malloc` chiffré ; zones visibles
dans une capture Tracy.

**Étude E0 — Comment démarre un moteur** : `FEngineLoop` d'Unreal (PreInit, Init, Tick), PlayerLoop d'Unity,
`Main::setup` et `Main::iteration` de Godot.

## Phase 1 — Fenêtre et premier triangle

| Milestone | Heures D. | Sessions | Échéance |
|---|---:|---:|---|
| M1.1 Fenêtre et boucle | 1,5 | 1 | 18/10/2026 |
| M1.2 Device NVRHI (Vulkan) et swapchain | 1,5 | 1 | 25/10/2026 |
| M1.3 Premier triangle | 1,5 | 1 | 01/11/2026 |
| ~~M1.4 Backend Direct3D 12~~ | — | — | **différé** (ADR-0011) |

**M1.1 — Fenêtre et boucle.** Fenêtre SDL3, boucle principale, événements, redimensionnement, mesure du
frame time ; ASan et UBSan en CI Linux.
*Critères* : redimensionnement et minimisation sans plantage ; frame time affiché ; zéro fuite signalée par
les sanitizers.

**M1.2 — Device NVRHI (Vulkan) et swapchain.** `DeviceManager` Vulkan inspiré de celui de Donut et adapté à
SDL3 : instance, device, queues, swapchain recréée au redimensionnement ; `nvrhi::vulkan::createDevice` ;
couche de validation NVRHI et validation layers Vulkan ; écran effacé à une couleur.
*Critères* : zéro erreur de validation sur 5 minutes avec redimensionnements ; temps de démarrage mesuré.

**M1.3 — Premier triangle.** Shaders Slang compilés au build en SPIR-V et DXIL, pipeline graphique, command
list NVRHI ; test de fumée headless sous lavapipe en CI Linux.
*Critères* : triangle affiché sous Linux ; zéro erreur de validation ; frame time CPU < 1 ms ; image du test de
fumée identique à la référence.

**M1.4 — différé (ADR-0011).** Le projet cible Linux d'abord : pas de backend Direct3D 12 tant qu'aucune
machine Windows n'est disponible. Le texte reste ici pour quand ce sera le cas.

*Contenu différé :* **Backend Direct3D 12.** `DeviceManager` D3D12 ; choix du backend au lancement (`--api vulkan|d3d12`) ;
test de fumée sous WARP (D3D12 logiciel) en CI Windows.
*Critères* : le même triangle sous les deux backends ; CI Windows verte ; binaire Windows de la CI lancé sous
Proton sur la machine de référence en `--api d3d12` (voir SPECS §10, « Vérification sous Windows »).

**Étude E1 — Les couches RHI** : déjà écrite ([E1-rhi.md](etudes/E1-rhi.md)), à relire pendant la phase.

## Phase 2 — 3D de base

| Milestone | Heures D. | Sessions | Échéance |
|---|---:|---:|---|
| M2.1 Caméra, meshes et binding sets | 2,5 | 2 | 15/11/2026 |
| M2.2 Textures | 1,5 | 1 | 22/11/2026 |
| M2.3 Hot-reload des shaders | 1,5 | 1 | 29/11/2026 |

**M2.1 — Caméra, meshes et binding sets.** Caméra 3D, depth buffer, meshes indexés, constantes par frame
(volatile constant buffers de NVRHI), instancing, timestamps GPU ; stratégie de binding (**ADR à écrire** :
binding sets ou bindless via descriptor tables).
*Critères* : 10 000 cubes instanciés à plus de 60 images/s en 1080p sur la machine de référence ; temps GPU
affiché.

**M2.2 — Textures.** Chargement stb_image, génération des mipmaps, samplers, filtrage anisotrope.
*Critère* : niveaux de mip vérifiés dans une capture RenderDoc.

**M2.3 — Hot-reload des shaders.** Surveillance des fichiers, recompilation à chaud via la bibliothèque Slang,
recréation des pipelines, repli si la compilation échoue.
*Critères* : modification visible en moins d'1 s sans redémarrer ; une erreur de compilation ne fait pas
planter (message dans le log).

**Étude E2 — Ressources GPU et shaders** : binding sets et bindless dans NVRHI, ShaderCompileWorker d'Unreal,
variantes de shaders d'Unity.

## Phase 3 — Scène et ECS

| Milestone | Heures D. | Sessions | Échéance |
|---|---:|---:|---|
| M3.1 Intégration de flecs et explorer | 1,5 | 1 | 06/12/2026 |
| M3.2 Transforms et hiérarchie | 1,0 | 1 | 13/12/2026 |
| M3.3 Boucle à pas fixe | 1,5 | 1 | 20/12/2026 |
| M3.4 Input par actions et caméra libre | 1,5 | 1 | 27/12/2026 |
| M3.5 Choix du jeu | 0,5 | 0 | 27/12/2026 |

**M3.1 — Intégration de flecs et explorer.** Monde flecs, composants de base, systèmes rangés par phases,
modules flecs ; le renderer dessine ce que contient le monde ; explorer web activé en Debug.
*Critères* : les entités de la démo sont visibles et modifiables dans l'explorer (flecs.dev/explorer) ; mise à
jour de 100 000 entités (Transform + Velocity) en moins d'1 ms.

**M3.2 — Transforms et hiérarchie.** Relations `ChildOf`, matrices monde calculées par une requête en cascade.
*Critère* : 100 000 entités sur 10 niveaux de profondeur recalculées en moins de 2 ms.

**M3.3 — Boucle à pas fixe.** Pipeline flecs dédié à la simulation, exécuté N fois par frame par un
accumulateur ; interpolation du rendu ; garde-fou contre la « spirale de la mort » (**ADR à écrire**).
*Critère* : test automatique montrant un état de simulation identique au bit près après N ticks, que le rendu
tourne à 30, 60 ou 144 images/s.

**M3.4 — Input par actions et caméra libre.** Actions et axes (clavier, souris, manette), configuration dans un
fichier, caméra libre.
*Critères* : une même action pilotée au clavier et à la manette ; changement de touches sans recompiler.

**M3.5 — Choix du jeu.** Une page de game design : genre, boucle de jeu, contenu minimal, ce que le moteur doit
savoir faire. Les phases 4 à 8 sont ensuite ajustées pour servir ce jeu.
*Critère* : la page est validée et la roadmap mise à jour en conséquence.

**Étude E3 — Modèles objets** : archetypes (flecs, Unity DOTS, Unreal Mass, Bevy) contre sparse sets (EnTT),
Actors/Components d'Unreal, GameObject d'Unity, Nodes de Godot.

## Phase 4 — Assets

| Milestone | Heures D. | Sessions | Échéance |
|---|---:|---:|---|
| M4.1 Import glTF | 2,0 | 2 | 10/01/2027 |
| M4.2 Base d'assets | 2,5 | 2 | 17/01/2027 |
| M4.3 Cuisson des assets | 3,0 | 2 | 31/01/2027 |
| M4.4 Hot-reload des assets | 1,5 | 1 | 07/02/2027 |

**M4.1 — Import glTF.** fastgltf : meshes, matériaux, textures et hiérarchie convertis en entités flecs.
*Critères* : la scène Sponza (Khronos glTF Sample Assets) s'affiche ; temps de chargement mesuré.

**M4.2 — Base d'assets.** GUID, fichiers `.meta`, registre, handles, comptage de références (**ADR à écrire**).
*Critères* : renommer ou déplacer un fichier ne casse aucune référence ; aucun chemin absolu dans les scènes.

**M4.3 — Cuisson des assets.** Outil hors ligne qui convertit vers un format binaire, textures KTX2 compressées
en BC7.
*Critères* : Sponza cuite charge au moins 5 fois plus vite que le glTF brut ; mémoire vidéo des textures mesurée
avant et après.

**M4.4 — Hot-reload des assets.**
*Critère* : une texture modifiée dans un logiciel externe est visible en moins de 2 s.

**Étude E4 — Pipelines d'assets** : `.uasset` et Derived Data Cache d'Unreal, `.meta` et `Library/` d'Unity,
`.import` et UID de Godot.

## Phase 5 — Rendu PBR

| Milestone | Heures D. | Sessions | Échéance |
|---|---:|---:|---|
| M5.1 PBR direct | 2,5 | 2 | 21/02/2027 |
| M5.2 HDR et tonemapping | 1,5 | 1 | 28/02/2027 |
| M5.3 Ombres en cascades | 2,5 | 2 | 14/03/2027 |
| M5.4 Éclairage d'environnement (IBL) | 2,5 | 2 | 21/03/2027 |
| M5.5 Culling et statistiques | 2,0 | 1 | 04/04/2027 |

**M5.1 — PBR direct.** Modèle metallic-roughness (Cook-Torrance), lumières directionnelle et ponctuelles ;
choix forward ou forward+ (**ADR à écrire**). Les passes de Donut servent de référence.
*Critère* : les modèles de test Khronos (par exemple MetalRoughSpheres) correspondent visuellement à la
visionneuse de référence.

**M5.2 — HDR et tonemapping.** Cible de rendu RGBA16F, exposition, tonemapping (AgX ou ACES).
*Critère* : captures comparatives avant et après.

**M5.3 — Ombres en cascades.** Shadow maps directionnelles, 4 cascades, filtrage PCF.
*Critère* : temps GPU de la passe d'ombres mesuré sur Sponza.

**M5.4 — IBL.** Irradiance, spéculaire préfiltré, table BRDF, calculés en compute shaders.
*Critère* : comparaison visuelle avec la visionneuse de référence.

**M5.5 — Culling et statistiques.** Frustum culling CPU, temps GPU par passe, compteurs de draw calls et de
triangles.
*Critère* : Sponza en PBR avec ombres et IBL à plus de 60 images/s en 1080p sur la machine de référence.

**Étude E5 — Forward, deferred, forward+** : les choix d'Unreal (deferred), d'Unity (URP et HDRP) et de Godot
(Forward+, Mobile, Compatibility).

## Phase 6 — Physique

| Milestone | Heures D. | Sessions | Échéance |
|---|---:|---:|---|
| M6.1 Intégration Jolt | 2,5 | 2 | 11/04/2027 |
| M6.2 Colliders, requêtes, debug draw | 2,0 | 1 | 25/04/2027 |
| M6.3 Character controller | 2,0 | 1 | 02/05/2027 |

**M6.1 — Intégration Jolt.** Monde physique, corps statiques et dynamiques, synchronisation flecs ↔ Jolt au pas
fixe.
*Critère* : 1 000 caisses en chute libre, pas de simulation sous 4 ms.

**M6.2 — Colliders, requêtes, debug draw.** Boîtes, sphères, capsules, meshes ; raycasts ; couches de
collision ; affichage de debug.
*Critère* : sélection d'un objet à la souris par raycast.

**M6.3 — Character controller.** `CharacterVirtual` de Jolt : pentes, marches.
*Critère* : se déplacer dans Sponza, escaliers compris.

**Étude E6 — La physique dans les moteurs** : Chaos (Unreal), PhysX (Unity), Jolt (Godot 4.4 et plus) ;
pourquoi c'est presque toujours une bibliothèque.

## Phase 7 — Éditeur

| Milestone | Heures D. | Sessions | Échéance |
|---|---:|---:|---|
| M7.1 ImGui et panneaux de debug | 1,5 | 1 | 09/05/2027 |
| M7.2 Réflexion et inspecteur | 2,0 | 1 | 16/05/2027 |
| M7.3 Sérialisation et undo/redo | 2,0 | 2 | 30/05/2027 |
| M7.4 Gizmos et picking | 2,5 | 2 | 06/06/2027 |
| M7.5 Play/Stop dans l'éditeur | 1,5 | 1 | 13/06/2027 |

**M7.1 — ImGui et panneaux de debug.** Renderer ImGui pour NVRHI (adapté de Donut), backend SDL3, panneaux de
statistiques et de profiling.
*Critère* : coût de l'UI inférieur à 0,5 ms par frame.

**M7.2 — Réflexion et inspecteur.** Réflexion des composants via l'addon meta de flecs (**ADR à écrire**), panneau de
hiérarchie, inspecteur de composants.
*Critère* : un nouveau composant devient éditable en une seule déclaration.

**M7.3 — Sérialisation et undo/redo.** Scènes en JSON via le sérialiseur de flecs, pattern Command.
*Critères* : sauvegarde puis chargement donnent une scène identique (test) ; 100 niveaux d'annulation.

**M7.4 — Gizmos et picking.** ImGuizmo, sélection à la souris par buffer d'identifiants.
*Critère* : sélection en une frame.

**M7.5 — Play/Stop.** Sauvegarde de la scène, simulation, restauration.
*Critère* : après Stop, la scène est identique à l'état d'avant Play (test).

**Étude E7 — Réflexion et éditeurs** : Unreal Header Tool, sérialisation d'Unity, `ClassDB` de Godot, addon meta
de flecs.

## Phase 8 — Audio et le jeu

| Milestone | Heures D. | Sessions | Échéance |
|---|---:|---:|---|
| M8.1 Audio | 1,5 | 1 | 20/06/2027 |
| M8.2 Le jeu (vertical slice) | 8,0 | 6 | 01/08/2027 |
| M8.3 Bilan v1 | 1,5 | 1 | 08/08/2027 |

**M8.1 — Audio.** miniaudio, composants AudioSource et AudioListener, spatialisation 3D.
*Critère* : 32 sons 3D simultanés sans coupure.

**M8.2 — Le jeu (vertical slice).** Le jeu choisi en M3.5, fait uniquement avec le moteur et l'éditeur.
*Critères* : 5 à 10 minutes de jeu ; binaires Windows et Linux produits par la CI et publiés en Release.

**M8.3 — Bilan v1.** Mesures finales, rétrospective estimé vs réel, roadmap v2.

**Étude E8 — Post-mortem du moteur.**

---

## Candidats v2

Render graph (frame graph de Frostbite, RDG d'Unreal) · job system multithread · animation squelettique
(ozz-animation) · GPU-driven rendering (draw indirect, mesh shaders, que NVRHI prend en charge) · ray tracing (aussi
pris en charge par NVRHI) · scripting (Lua, C# ou WebAssembly) · réseau · streaming de monde.

## ADR à venir

| ADR | Sujet | Milestone |
|---|---|---|
| 0008 | Gestion d'erreurs (exceptions ou codes de retour) | M0.3 |
| 0009 | Stratégie de binding (binding sets ou bindless) | M2.1 |
| 0010 | Boucle à pas fixe et interpolation | M3.3 |
| 0011 | Identifiants d'assets et format `.meta` | M4.2 |
| 0012 | Forward ou forward+ | M5.1 |
| 0013 | Réflexion des composants (addon meta de flecs) | M7.2 |

## Numérotation des ADR

**La roadmap ne pré-attribue plus de numéros d'ADR.** La v0.1 en annonçait cinq à l'avance ; l'aller-retour par
Rust en a consommé deux au passage (0010 et 0011), et toute la suite a glissé. Un numéro se prend **au moment
d'écrire l'ADR**, en suivant le dernier existant dans `docs/adr/`.

## Recalibrage

À la clôture de chaque phase, Claude calcule le ratio **heures passées / heures estimées** de la phase, l'inscrit
dans le journal, puis :

- si le ratio est entre 0,8 et 1,25 : rien à changer ;
- sinon : les estimations des phases restantes sont multipliées par ce ratio, et les échéances des milestones
  GitHub sont décalées en conséquence (dans une PR `docs(roadmap): recalibrage phase N`).

**Le ratio se calcule sur le temps total de Donnovan**, pas sur sa seule relecture : pilotage, questions et
décisions en font partie (voir la définition des « Heures Donnovan » plus haut). La phase 0 l'a appris à ses
dépens — mesurée d'abord à 3,0 h en ne comptant que les relectures, contre **4,9 h réelles**. Le ratio erroné de
0,50 aurait amputé la roadmap de 30 % sans raison.

### Phase 0 — ratio 0,83, aucun recalibrage

| | Estimé | Passé | Ratio |
|---|---:|---:|---:|
| Phase 0 (14 issues, 5 milestones) | 6,0 h | **5,0 h** | **0,83** |

Dans la fourchette 0,8–1,25 : les estimations des phases 1 à 8 sont conservées telles quelles. À réexaminer à
la clôture de la phase 1, qui sera le premier échantillon de vrai code de rendu — la phase 0 était faite de
specs, d'ADR et de configuration, et n'en dit pas grand-chose.
