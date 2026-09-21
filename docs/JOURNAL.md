# Journal de bord

Une entrée par session de travail, la plus récente en haut. Les heures Donnovan sont celles qu'il déclare ;
les chiffres de performance viennent de commandes versionnées, sur la machine de référence.

## Format d'une entrée

```
## AAAA-MM-JJ — Mx.y — titre court
- Temps Donnovan : x h (estimé y h)
- Sessions Claude Code : n
- Fait : …
- Mesures : critère → valeur (commande)
- Décisions : ADR-XXXX …
- Écarts et problèmes : …
- Prochaine étape : …
```

## Cumul

| Phase | Estimé (h) | Passé (h) | Ratio |
|---|---:|---:|---:|
| 0 | 6,0 | **5,0** | **0,83** |
| 1 | 4,5 | **3,0** | **0,67** |

---

## 2026-09-21 — M2.1 — Test de fumée du cube (#41, 3/3)

- Temps Donnovan : à renseigner (compté avec #41)
- Sessions Claude Code : 1
- Fait : le test de fumée dessine le triangle **ou** le cube (`levain_smoke_render triangle|cube`, deux tests
  ctest) ; référence `tests/data/cube.ppm`, 64 × 64, cube à 35° autour de (1, 1, 0).
- Mesures (critère de #41 : « un cube tourne avec un depth buffer correct ») :
  - **référence regardée** : faces du dessus (verte) et avant (bleue) pleines, une tranche de la face gauche,
    aucune face intérieure visible ;
  - **élimination des faces** : désactivée, 0 pixel différent de la référence — elle ne retire que des faces
    cachées ; **sens des faces inversé**, 454 pixels différents — le test l'attrape ;
  - la réécriture du tableau des sommets (coins nommés, pour la lisibilité) passe contre la référence générée
    avant elle : géométrie inchangée.
- Prochaine étape : #42 — instancing et timestamps GPU.

## 2026-09-21 — M2.1 — Le cube : mesh indexé, depth buffer, constantes par frame (#41, 2/3)

- **Temps Donnovan : 0,33 h** (20 min déclarées, relecture de #74 ; le temps de #73 reste à déclarer)
- Sessions Claude Code : 1
- Fait : `Mesh`, `createMesh`, `createCube` ; `MeshPass` — input layout, binding layout de frame dans `space0`
  (ADR-0013) avec un volatile constant buffer, depth buffer `D32`, élimination des faces arrière ;
  `shaders/mesh.slang`, compilé avec `-matrix-layout-column-major` ; le sandbox dessine un cube qui tourne.
- Mesures :
  - **zéro erreur de validation** au lancement, cube compris (Debug, RADV) : la convention d'espaces de
    l'ADR-0013 tient avec un binding layout (`setRegisterSpaceAndDescriptorSet(0)`) ;
  - le rendu à l'image près et le sens des faces sont vérifiés par le test de fumée du cube (#41, 3/3).
- Écarts et problèmes :
  - Deux pièges nommés dans le code : `VertexBufferBinding` et `IndexBufferBinding` n'ont pas de valeur par
    défaut pour leur slot, leur format et leur décalage ; la disposition des matrices est imposée à `slangc`
    plutôt que laissée à son défaut.
- Prochaine étape : le test de fumée du cube (#41, 3/3).

## 2026-09-21 — M2.1 — Caméra, chargement des shaders et cache CI (#41, 1/3)

- Temps Donnovan : à renseigner (estimé 0,65 h pour #41, après recalibrage)
- Sessions Claude Code : 1
- Fait : première des trois PR de #41. GLM (port vcpkg 1.0.3) ; `Camera` et `viewProjectionOf` dans `render`,
  avec deux tests ; chargement des shaders sorti de `triangle.cpp` dans `src/shader.cpp`, pour servir aussi la
  passe des meshes ; cache des téléchargements de vcpkg en CI, à la demande de Donnovan.
- Mesures :
  - **le piège de la profondeur est testé** : `viewProjectionOf` range la profondeur de 0 à 1 ; contre-test avec
    la projection d'OpenGL (`perspectiveRH_NO`), le plan proche tombe à −1 et le test échoue ;
  - 28 tests.
- Écarts et problèmes :
  - **#41 découpée en trois PR** : la version d'un bloc faisait 640 lignes, et ce n'était pas du code Vulkan
    indivisible. Suivent le cube (mesh indexé, depth buffer, passe) et son test de fumée, déjà écrits et vérifiés.
  - Un `git stash` a refusé de remiser des fichiers marqués `git add -N` : sauvegarde complète d'abord, index
    vidé, puis remisage. Rien de perdu.
  - **Premier passage de la CI rouge sur les trois jobs** : le script de bootstrap de vcpkg refuse de démarrer
    si le dossier `VCPKG_DOWNLOADS` n'existe pas, et je ne le créais qu'après. Second commit : restauration et
    création du dossier avant le bootstrap.
- Prochaine étape : le cube (#41, 2/3).

## 2026-09-21 — M2.1 — ADR-0013 : binding sets (#40)

- Temps Donnovan : à renseigner (estimé 0,35 h pour #40, après recalibrage)
- Sessions Claude Code : 1
- Fait : les deux modèles de liaison de NVRHI écrits côte à côte (C++ et Slang), puis un sondage. **Donnovan
  retient les binding sets** ; passer au bindless demandera un nouvel ADR, justifié par une mesure. ADR-0013 écrit,
  avec la convention qui en découle : un binding layout par fréquence de changement (`space0` frame, `space1` passe,
  `space2` matériau), `setRegisterSpaceAndDescriptorSet` pour que l'espace devienne le descriptor set sous Vulkan.
- Écarts et problèmes : la convention d'espaces est lue dans le code de NVRHI, pas encore vérifiée par la
  validation ; elle le sera avec les premières constantes de M2.1 (#41).
- Prochaine étape : #41 — caméra 3D, depth buffer et meshes indexés.

## 2026-09-21 — Recalibrage après la phase 1

- Temps Donnovan : compté dans les 3,0 h de la journée
- Fait : ROADMAP v0.5 — estimations des phases 2 à 8 multipliées par le ratio de la phase 1 (0,67), arrondies au
  quart d'heure, échéances recalculées à 1,5 h par semaine (script dans la PR). **58,5 h → 39,0 h restantes ;
  68 h → 47 h au total ; le jeu jouable passe du 01/08/2027 au 21/03/2027.**
- Décisions : **Donnovan retient l'option 1**, la règle appliquée telle quelle (ratio 0,67), parmi trois : le ratio
  cumulé des phases 0 et 1 (0,76), ou attendre la phase 2. Le prochain point de contrôle est la clôture de la
  phase 2.
- Après fusion : échéances des milestones GitHub M2.1 à M8.3 et estimations des issues des phases 2 et 3 (#40 à
  #47, #61 à #69) mises à jour au même ratio.
- Prochaine étape : M2.1, en commençant par l'ADR de liaison (#40).

## 2026-09-21 — Phase 1 — Clôture (M1.3 compris)

- **Temps Donnovan pour la journée : 3,0 h** (« 3 h grand max », relecture de #60 et sondages sur E1 compris), le
  seul chiffre déclaré comme total. Réparti :
  - M1.1 : 1,25 h, déjà réconcilié sur le total du matin ;
  - l'après-midi, 1,75 h : les morceaux déclarés (#12 : 20 min, #13 : 15, #14 : 20, #15 : 15) sont augmentés au
    prorata ; **#16 et #17 n'ont jamais été chiffrés à part, et leurs 0,15 h chacun sont une répartition de ma
    part**. Le total de la phase ne dépend pas de cette répartition.
- Définition de « terminé » pour M1.3 (SPECS §9) : démo lançable sous Linux ; critères mesurés et consignés ; CI
  verte, zéro erreur de validation ; README de `render`, `core` et `gpu` à jour ; board renseigné ; tag `m1.3`.
  Étude de la phase : E1, écrite avant la phase, relue et corrigée (voir plus bas).

### Critères de M1.3

| Critère (ROADMAP) | Mesuré | Commande |
|---|---|---|
| Triangle affiché sous Linux | oui, relu à l'image près par le test de fumée, dans le bon sens | `ctest -R smoke` |
| Zéro erreur de validation | 0, dans toutes les configurations | sandbox, CI |
| Frame time CPU < 1 ms | **0,090 ms** d'enregistrement et de soumission, 0,149 ms pour la frame hors attente de l'écran (Debug, validation) | `tools/tracy-capture.sh` |
| Image du test de fumée identique à la référence | oui sous lavapipe en CI, à ±2 près par canal, avec une référence générée sur RADV | CI |

### Phase 1 — le ratio

| Milestone | Estimé | Passé |
|---|---:|---:|
| M1.1 Fenêtre et boucle | 1,5 h | 1,25 h |
| M1.2 Device NVRHI et swapchain | 1,5 h | 0,72 h |
| M1.3 Premier triangle | 1,5 h | 1,03 h |
| **Phase 1** | **4,5 h** | **3,0 h** — ratio **0,67** |

**Hors de la fourchette 0,8–1,25, et cette fois ce n'est pas une erreur de mesure** : le total vient de Donnovan,
qui relit les diffs par petits morceaux entre deux tâches, et plus vite que prévu. La règle de la ROADMAP s'applique :
les estimations des phases 2 à 8 sont multipliées par 0,67 et les échéances avancées. **Proposé dans une PR à part,
`docs(roadmap): recalibrage phase 1`, que Donnovan valide ou amende.** Deux réserves à y peser : l'échantillon tient
en une journée intense, et la phase 0 donnait 0,83.

### #17 — lecture de l'étude E1, par sondages

Donnovan a demandé à être interrogé plutôt que de relire. Huit questions, **5 bonnes réponses** : durée de vie,
suivi d'états, niveaux d'abstraction, threads d'Unreal, frontière de NVRHI sont acquis. Les trois erreurs portent
sur la liaison des ressources (décalages de binding, binding sets contre bindless, volatile constant buffers), qui
arrive en phase 2 ; réponses archivées dans `docs/QA.md`. En relisant E1, j'y ai trouvé deux erreurs, corrigées :
`nvrhi::validation::createDevice` n'existe pas (c'est `createValidationLayer`), et la compilation des shaders ne
passe plus par ShaderMake (ADR-0005, amendement).

### Phase 3 détaillée

Neuf issues créées, #61 à #69, sur les milestones M3.1 à M3.5, avec estimation et phase sur le board (les
estimations suivent la ROADMAP actuelle ; le recalibrage les mettra à jour s'il est validé).

### Ce que la phase 1 a appris

- **Les garde-fous ont payé** : la boucle de CI trop courte (deux fois), la validation qui a arrêté le premier
  triangle sur `shaderDrawParameters`, `TRACY_ENABLE` désactivé par Tracy 0.14 — chaque fois, un contrôle a
  échoué bruyamment au lieu de rester vert.
- **La mesure du temps a encore dérivé**, sous une troisième forme : des morceaux « depuis ta dernière réponse »
  qui oubliaient le temps entre deux. Seul le total de la journée l'a rattrapée.
- **Le code Vulkan de base ne se découpe pas en PR de 400 lignes** ; le reste, si. Deux exceptions admises,
  une découpe faite (#58, #59).

- Prochaine étape : la PR de recalibrage, puis M2.1 — caméra, meshes et binding sets, en commençant par l'ADR de
  liaison (#40), là même où le quiz a montré les lacunes.

## 2026-09-21 — M1.3 — Test de fumée du rendu (#16)

- **Temps Donnovan : 0,15 h** (non chiffré à part : répartition de ma part dans le total de la journée, voir la clôture de la phase 1 ; estimé 0,25 h)
- Sessions Claude Code : 1
- Fait : `tests/smoke_triangle.cpp`, enregistré dans ctest (`smoke.triangle`) : fenêtre offscreen, triangle dessiné
  dans une texture de 64 × 64, recopié dans une texture lisible par le CPU, comparé à `tests/data/triangle.ppm`
  à ±2 près par canal ; `LEVAIN_UPDATE_REFERENCE=1` réécrit la référence.
- Mesures :
  - **le triangle est dessiné, et dans le bon sens** : référence générée sur RADV, validation active, zéro
    message ; regardée agrandie (rouge en bas à gauche, vert au sommet, bleu en bas à droite). C'est la première
    preuve à l'image que le rendu fonctionne, sans capture d'écran du bureau ;
  - **le test échoue si le triangle ne s'affiche plus** (critère de #16) : dessin retiré, test rouge ; le
    triangle couvre 512 pixels sur 4 096, soit 12,5 %, tous différents sans lui ;
  - 26 tests verts en Debug, Release et ASan (avec RADV préchargé).
- Écarts et problèmes :
  - **La référence vient de RADV, la CI compare avec lavapipe.** La tolérance de ±2 absorbe les écarts
    d'interpolation ; un écart de couverture sur les bords du triangle ferait échouer le test. Non vérifiable ici,
    lavapipe n'étant pas installé sur la machine de référence : la CI de cette PR le dira.
  - clang-tidy refusait une multiplication en `int` convertie en décalage de pointeur : calculée en `size_t`.
- Prochaine étape : clôture de M1.3, puis de la phase 1 (ratio, recalibrage, détail de la phase 3).

## 2026-09-21 — M1.3 — Premier triangle (#15)

- **Temps Donnovan : 0,31 h** (15 min déclarées, relecture de #59 comprise ; réconcilié, voir la clôture de la phase 1 ; estimé 0,5 h)
- Sessions Claude Code : 1
- Fait : module `engine/render` et `TrianglePass`, qui ne connaît que `nvrhi::IDevice` (SPIR-V ou DXIL choisi
  d'après l'API du device) ; `readFile` dans `core` ; `swapchainFormat` dans `gpu` ; le triangle dessiné par le
  sandbox, avec des zones Tracy pour mesurer le travail CPU d'une frame.
- Mesures :
  - **frame time CPU** (critère de M1.3, < 1 ms) : **0,090 ms** pour enregistrer et soumettre une frame, **0,149 ms**
    pour la frame entière hors attente de l'écran, en Debug avec validation ; un pic isolé à 1,33 ms sur 19 945
    frames (`SDL_VIDEO_DRIVER=offscreen ./tools/tracy-capture.sh 3 captures/m1.3.tracy`, zones `commandes` et
    `rendu`) ;
  - **zéro message de validation** : Debug sous Wayland, X11 et offscreen, Release, ASan avec RADV préchargé ;
  - 25 tests (23 + 2 pour `readFile`).
- Écarts et problèmes :
  - **Première vraie erreur de validation attrapée par l'assertion** : le SPIR-V de Slang déclare
    `DrawParameters` pour traduire `SV_VertexID`, et le device n'activait pas `shaderDrawParameters`. Corrigé
    dans `device_vk.cpp` ; la raison est dans l'ADR-0005.
  - **Je n'ai pas vu le triangle** : pas de capture d'écran (règle du skill `build`). Ce qui est vérifié :
    pipeline créé, draw enregistré, zéro erreur de validation. La preuve à l'image près viendra du test de fumée
    (#16), qui relit les pixels ; en attendant, c'est à Donnovan de le regarder.
  - **Bridage passager de l'environnement** : pendant quelques minutes, 20 images/s très régulières sous Wayland
    et une fenêtre X11 qui ne s'ouvrait plus, puis tout est revenu sans changement de code. Noté dans
    `build/GOTCHA.md`, avec la parade : mesurer en offscreen.
  - **CI rouge en Release, et c'est le garde-fou de M1.2 qui a sonné** : le démarrage du runner a pris ~10 s
    (2,3 s rien que pour le device, contre 0,9 s d'habitude), et le SIGTERM des 8 s est tombé avant la première
    frame — boucle de 0,5 s, contrôle « ≥ 1 s » en échec. En M1.1, j'avais préféré `timeout` à une option du
    sandbox, pour écrire moins de code ; les faits me donnent tort. Parade : `--seconds N`, compté depuis le
    premier tour de boucle, et `timeout` à 60 s comme simple filet (arguments testés : 2 et 0,5 acceptés,
    `abc` et `-1` refusés avec le code 2).
- Prochaine étape : #16, le test de fumée sous lavapipe qui compare l'image rendue à une référence.

## 2026-09-21 — M1.3 — Compilation des shaders Slang (#14)

- **Temps Donnovan : 0,42 h** (20 min déclarées, relecture de #58 comprise ; réconcilié, voir la clôture de la phase 1 ; estimé 0,5 h)
- Sessions Claude Code : 1
- Fait : compilation des shaders Slang au build par des commandes CMake et `slangc` (ports vcpkg `shader-slang`
  et `directx-dxc`), en SPIR-V et en DXIL, choix de Donnovan inscrits dans l'ADR-0005 ; `shaders/triangle.slang`,
  qui servira au premier triangle (#15).
- Mesures :
  - **une erreur de shader fait échouer le build** : `error[E30015]: undefined identifier`, code 255 (faute
    injectée depuis une copie, puis retirée) ;
  - **seul un shader modifié est recompilé** : après `touch shaders/triangle.slang`, `ninja -n` ne liste que ses
    deux points d'entrée ; sans modification, `no work to do` ;
  - **DXIL bien formé** : deux tests `dxil.*` (désassemblage par `dxc -dumpbin`), le fichier porte un hash de
    shader.
- Écarts et problèmes :
  - Mon premier contrôle « seul un shader modifié est recompilé » ne prouvait rien : le reste du projet n'était
    pas encore recompilé, et `ninja -n` listait aussi du C++. Refait après un build complet.
  - PR découpée par issue : la version commune avec le triangle faisait 504 lignes, et ce n'était pas un bloc
    Vulkan indivisible (règle n°2).
- Prochaine étape : #15, le premier triangle.

## 2026-09-21 — M1.2 — Clôture

- **Temps Donnovan pour M1.2 : 0,72 h**, réconcilié à la clôture de la phase 1 (0,58 h déclarées d'abord, puis 0,88 h,
  puis la répartition finale) ; ce qui suit décrit la version provisoire. 0,58 h déclarées (10 + 10 + 15 min) : ce sont des réponses
  « depuis la dernière fois », que la clôture de M1.1 a montrées incomplètes. À réconcilier avec le total de la
  journée en fin de session (skill `session`).
- Définition de « terminé » (SPECS §9) : démo lançable sous Linux (Windows hors périmètre, ADR-0011) ; critères
  mesurés et consignés ; CI verte, zéro erreur de validation ; README de `gpu` et de `platform` à jour ; board
  renseigné ; tag `m1.2` et release. Pas d'étude : elle vient à la fin de la phase 1 (E1, déjà écrite).

### Critères du milestone

| Critère (ROADMAP) | Mesuré | Commande |
|---|---|---|
| Zéro erreur de validation sur 5 minutes avec redimensionnements | **0 message, 584 redimensionnements**, sandbox vivant | `./tools/kwin-window-smoke.sh 300` |
| Temps de démarrage mesuré | device créé en **30 à 40 ms** (RADV) ; 0,9 à 1,3 s en CI (lavapipe) | sandbox, « device créé en » |
| En plus, reporté de M1.1 : minimisation sous Wayland | la boucle s'endort, 0 ms de CPU | même script |

### Temps (provisoire)

| Issue | Estimé | Passé déclaré |
|---|---:|---:|
| #12 Device Vulkan et NVRHI | 1,0 h | 0,33 h |
| #13 Swapchain et écran effacé | 0,5 h | 0,25 h |
| **M1.2** (ROADMAP) | **1,5 h** | **0,58 h** — ratio 0,39, à réconcilier |

### Décisions

- **ADR-0012** : vk-bootstrap pour le device Vulkan, choisi par Donnovan sur question posée avant l'implémentation.
- **Règle n°2** : du code Vulkan qui forme un bloc peut dépasser les ~400 lignes s'il reste lisible et que l'écart
  est signalé (Donnovan, après relecture de #56). Inscrit dans `AGENTS.md`.

### Ce que M1.2 a appris

- **Une panne silencieuse de plus, en CI** : le démarrage sur lavapipe mangeait tout le délai du sandbox, qui ne
  testait plus la boucle en restant vert. La CI exige maintenant une boucle d'au moins une seconde.
- **Les pièges de NVRHI viennent de sa façon d'être compilé et enveloppé** : dispatcher de Vulkan-Hpp à définir
  soi-même (bibliothèque statique), sémaphores accessibles seulement sous l'enveloppe de validation.
- **Mon propre `GOTCHA.md` ne sert que s'il est relu** : le piège `[[maybe_unused]]`, inscrit le matin, m'a coûté
  un build l'après-midi. Relu avant #13, il n'a pas resservi.

- Prochaine étape : M1.3 — premier triangle (shaders Slang, pipeline graphique, test de fumée sous lavapipe).

## 2026-09-21 — M1.2 — Swapchain, redimensionnement et écran effacé (#13)

- **Temps Donnovan : 0,31 h** (15 min déclarées, relecture de #56 comprise ; réconcilié, voir la clôture de la phase 1 ; estimé 0,5 h)
- Sessions Claude Code : 1
- Fait : swapchain Vulkan (vk-bootstrap) dont les images sont enveloppées en textures NVRHI ; reconstruction dès
  que la taille de la fenêtre change ; sémaphores d'acquisition et de présentation ; deux frames en vol au plus
  (*event queries* NVRHI) ; présentation FIFO ; chaque frame efface l'écran à une couleur ; `windowPixelSize`
  dans `platform` ; redimensionnements en boucle dans `tools/kwin-window-smoke.sh` ; réponse sur Windows et
  Direct3D 12 archivée dans `docs/QA.md`.
- Mesures (machine de référence, Wayland sauf mention) :
  - **5 minutes de redimensionnements, validation active : 584 changements de taille, zéro message de
    validation**, sandbox toujours vivant, code 0 (`./tools/kwin-window-smoke.sh 300`, sandbox en Debug) ;
  - **la fenêtre apparaît sous Wayland**, et KWin lit son titre : `Levain - 8.333 ms (min 7.176, max 9.490) -
    120 images/s` (`gdbus … krunner1.Match Levain`) ;
  - **minimisation sous Wayland** (reportée de M1.1) : « masquée » puis « visible », 0 ms de CPU en 2 s
    minimisée ;
  - **CPU visible : 40 à 50 ms en 2 s, contre 2 010 ms** avant la swapchain : la présentation FIFO cadence la
    boucle sur les 120 Hz de l'écran (600 frames en 5,0 s) ;
  - zéro message de validation aussi sous X11, en offscreen (surface headless, comme la CI), en Release, et
    sous ASan avec RADV préchargé (code 0).
- Écarts et problèmes :
  - **`queueWaitForSemaphore` n'existe pas sur `nvrhi::IDevice`**, seulement sur `nvrhi::vulkan::IDevice`, que
    l'enveloppe de validation n'expose pas. La swapchain garde donc le device Vulkan de NVRHI, et le moteur
    l'enveloppe, comme Donut.
  - **Les images de la swapchain sont des textures NVRHI** : elles doivent disparaître avant le device NVRHI.
    La swapchain devient le troisième membre de `GpuDevice`, déclaré en dernier pour être détruit en premier.
  - clang-tidy refuse une constante globale `nvrhi::Color` (constructeur non `noexcept`) : rendue locale.
  - En offscreen, la surface headless ne cadence rien : ~11 000 images/s. Sans conséquence, la CI vérifie la
    durée de la boucle, pas sa fréquence.
- Prochaine étape : clôture de M1.2 (critères tenus : 5 minutes sans erreur de validation, temps de démarrage
  mesuré), puis M1.3 — premier triangle.

## 2026-09-21 — M1.2 — Device Vulkan et NVRHI (#12)

- **Temps Donnovan : 0,41 h** (10 + 10 min déclarées, relectures de #54 et #55 ; réconcilié, voir la clôture de la phase 1 ; estimé 1,0 h)
- Sessions Claude Code : 1
- Fait : module `engine/gpu` — instance, surface et device Vulkan créés avec vk-bootstrap (ADR-0012), puis device
  NVRHI par-dessus ; couches de validation Vulkan et couche de validation NVRHI en Debug, messages redirigés vers
  nos logs, et arrêt sur assertion à la première erreur ; nom du GPU et version du pilote journalisés ; temps de
  création mesuré dans le sandbox ; lavapipe et couches de validation installés en CI.
- Mesures :
  - GPU choisi : **AMD Radeon RX 9070 XT (RADV GFX1201), pilote radv Mesa 26.2.3, Vulkan 1.4.354** — le
    discret, pas l'iGPU (`SDL_VIDEO_DRIVER=x11 ./build/linux-debug/sandbox/levain_sandbox`) ;
  - **device créé en 30 à 40 ms**, validation comprise (même commande, sous Wayland et X11) ;
  - **zéro message des couches de validation** au lancement. Les deux seules erreurs journalisées viennent du
    loader, qui signale la couche Lossless Scaling cassée (SPECS §10) ;
  - validation réellement active : couche `VK_LAYER_KHRONOS_validation` insérée en Debug, absente en Release
    (`VK_LOADER_DEBUG=layer`) ;
  - **contre-tests** : un buffer Vulkan de taille 0 (`VUID-VkBufferCreateInfo-size-00912`) et une texture NVRHI
    de largeur 0 arrêtent chacun le programme sur l'assertion (code 133, SIGTRAP) ;
  - sanitizers sur le vrai GPU : 21 tests verts ; le sandbox signale 128 octets alloués par RADV, un faux
    positif (voir ci-dessous) ; **code 0 sous Wayland avec RADV préchargé**.
- Écarts et problèmes :
  - **Fuite de 128 octets signalée dans RADV**, même famille que libX11 en M1.1 : le loader décharge le pilote
    à la destruction de l'instance. Isolé par élimination : elle disparaît quand RADV reste chargé, persiste
    sans couche de validation (build Release instrumenté) et sans `device_select`.
  - **Le piège `[[maybe_unused]]` inscrit le matin même dans `build/GOTCHA.md`** a resservi dès l'après-midi :
    le build Release a échoué sur `isValidationError`, qui ne sert qu'à une assertion. Il aurait fallu relire le
    fichier avant d'écrire, comme le demande `AGENTS.md`.
  - Pour retirer une faute injectée, un `git checkout` du fichier a effacé l'intégration du device dans le
    sandbox, pas encore commitée. Réécrite ; la parade est dans `build/GOTCHA.md`.
  - Sous X11, précharger RADV et libX11 ensemble bloque `SDL_CreateWindow` (`XIfEvent` attend un `MapNotify`).
    Configuration de diagnostic seulement ; noté.
  - **CI verte du premier coup sur lavapipe** (llvmpipe, Mesa 25.2.8, Vulkan 1.4.318), sans message de
    validation ni fuite. Mais **la boucle n'y tournait plus** : le device y met 0,9 à 1,3 s à se créer, et le
    démarrage complet mangeait les 3 s du délai — la création est journalisée après le SIGTERM. L'étape restait
    verte sans rien tester de la boucle : une panne silencieuse de plus. Parade : délai porté à 8 s, et le
    sandbox journalise la durée de sa boucle, que la CI exige d'au moins une seconde (contre-test de
    l'expression : 0,4 s refusé, 1,2 s accepté).
  - PR découpée : l'ADR-0012 et la correction du temps de M1.1 sont partis d'abord (#54), règle n°3.
- Prochaine étape : #13 — swapchain, redimensionnement et écran effacé, avec les points reportés de M1.1.

## 2026-09-21 — M1.1 — Clôture

- **Temps Donnovan pour M1.1 : 1,25 h** — total de la journée déclaré par Donnovan (« environ 1 h 15 depuis ce
  matin »). Les réponses données après chaque PR ne faisaient que 40 min (20 + 10 + 10) : elles oubliaient le
  temps passé entre deux, en allers-retours. L'écart est réparti au prorata sur les issues.
- Définition de « terminé » (SPECS §9) : 6 critères sur 7 remplis. Démo lançable sous Linux (Windows hors
  périmètre, ADR-0011) ; critères chiffrés mesurés et consignés ; CI verte, avec trois checks requis ; README de
  `platform` et de `core` à jour ; board renseigné ; tag `m1.1` et release publiés. Pas d'étude : elle vient à
  la fin de la phase.

### Critères du milestone

| Critère (ROADMAP) | Mesuré | Commande |
|---|---|---|
| Redimensionnement et minimisation sans plantage | oui, sous X11 ; **Wayland reporté en M1.2** (#13), la fenêtre n'y apparaît qu'avec la première image | `tools/kwin-window-smoke.sh` |
| Frame time affiché | oui, dans le titre, lu par KWin | `gdbus … krunner1.Match Levain` |
| Zéro fuite signalée par les sanitizers | oui, job `linux-asan` requis sur `main` | CI |
| En plus : capture Tracy (#38) | 1,8 µs par frame, dont 85 % à pomper les événements SDL | `tools/tracy-capture.sh` |

### Temps

| Issue | Estimé | Passé |
|---|---:|---:|
| #10 Fenêtre, boucle, événements | 0,75 h | 0,63 h |
| #11 Frame time et sanitizers | 0,75 h | 0,31 h |
| #38 Capture Tracy (reportée de M0.3) | 0,25 h | 0,31 h |
| #32 Issues de phase 1 après l'ADR-0010 | 0,25 h | 0 h — fait en phase 0, déjà compté dans ses 5,0 h |
| **M1.1** (estimation de la ROADMAP) | **1,5 h** | **1,25 h** — ratio **0,83** |

Ratio dans la fourchette 0,8–1,25, le même que celui de la phase 0. La première version de cette entrée
annonçait 0,44, calculé sur les seuls morceaux déclarés : **la même erreur de mesure qu'en phase 0, sous une
autre forme**. La question « depuis ta dernière réponse » laisse tomber le temps passé entre deux ; seule la
question sur le total de la journée l'a rattrapé.

### Ce que M1.1 a appris

- **Trois pannes silencieuses de plus, toutes attrapées cette fois** : le titre qui n'atteignait jamais l'écran
  sous X11 (bug SDL), la fenêtre invisible sous Wayland, et Tracy 0.14 qui compilait un profilage vide. La règle
  « un contrôle échoue bruyamment » est devenue la règle n°7, et chacun de ces cas a son garde-fou ou son
  assertion.
- **Les instructions ont changé de forme** : `AGENTS.md` et quatre skills avec leurs `GOTCHA.md`, à la demande de
  Donnovan. Ils ont servi dès la PR suivante, et j'y ai ajouté deux pièges le jour même.
- **Un incident** : une capture d'écran qui a saisi le navigateur de Donnovan au lieu du profileur. Supprimée ;
  la règle est inscrite. Le serveur MCP de Tracy, signalé par Donnovan, rendra les captures d'écran inutiles.

- Prochaine étape : M1.2 — `DeviceManager` Vulkan pour NVRHI (#12), puis swapchain et écran effacé (#13, avec les
  trois points reportés de M1.1). Mettre en place le serveur MCP de Tracy au passage.

## 2026-09-21 — M1.1 — Capture Tracy de la vraie boucle (#38)

- **Temps Donnovan : 0,31 h** (10 min déclarées ; 0,31 h après réconciliation, voir la clôture ; estimé 0,25 h)
- Sessions Claude Code : 1 (la même que #10, #11 et #51)
- Fait : port overlay `ports/tracy`, Tracy **0.14.1 client seul** (51 lignes, contre 759 pour le
  port officiel et ses quatre patches, qui ne concernent que les outils) ; garde-fou CMake sur `TRACY_ENABLE` ;
  outils 0.14.1 officiels dans `~/.local/opt/tracy-0.14.1/`, avec l'accord de Donnovan ; script
  `tools/tracy-capture.sh` ; zone `titre` dans la boucle ; amendement de l'ADR-0007 (ports overlay) ; `ports/`
  dans la clé du cache CI.
- Mesures (`SDL_VIDEO_DRIVER=x11 ./tools/tracy-capture.sh 3 captures/m1.1.tracy`) :
  - **1 807 095 frames en 3,3 s** : ~1,8 µs par frame, ~550 000 images/s avec Tracy (~820 000 sans) ;
  - **`événements` : 85,2 % du temps**, 1 555 ns en moyenne (de 1 190 ns à 254 709 ns). La frame est donc
    presque entièrement le pompage des événements de SDL, la seule chose que fait la boucle avant M1.2 ;
  - `titre` : 3 appels, 41,5 µs en moyenne, une fois par seconde ;
  - client actif : il écoute sur `*:8086` (`ss -ltnp`), `TRACY_NO_EXIT=1` l'empêche de sortir (code 137 au
    délai), 1 046 symboles `tracy::` dans le binaire (`nm -C | grep -c`) ;
  - release officielle : SHA-256 identique à celle publiée par GitHub (`gh api …/releases/tags/v0.14.1`) ;
  - vcpkg `master` toujours en 0.13.1 au 21/09 (troisième critère de l'issue).
- Écarts et problèmes :
  - **Tracy 0.14 a fait passer `TRACY_ENABLE` de ON à OFF par défaut.** Le premier build profilé compilait,
    mais sans aucun symbole `tracy::` : un profilage vide, sans un mot. Le port force l'option, et
    `engine/core/CMakeLists.txt` refuse de configurer sans elle. Contre-test : l'option retirée, CMake échoue.
  - **Capture d'écran ratée, et un incident.** Pour photographier le profileur, j'ai demandé le focus à KWin puis
    lancé `spectacle -a` (fenêtre active). KWin n'a pas donné le focus : l'image montrait le navigateur de
    Donnovan, sur une page de connexion. Supprimée aussitôt, jamais commitée ni envoyée. Règle inscrite dans le
    skill `build` : pas de capture d'écran du bureau, on la demande à Donnovan.
  - `pkill -f <motif>` a tué le shell qui l'exécutait, sa ligne de commande contenant le motif.
- Prochaine étape : capture d'écran de la timeline par Donnovan, puis clôture de M1.1.

## 2026-09-21 — M1.1 — AGENTS.md et skills

- Temps Donnovan : compté avec #11 (10 min pour les relectures de #50 et #51)
- Sessions Claude Code : 1 (la même que #10 et #11)
- Fait : à la demande de Donnovan, `CLAUDE.md` (182 lignes) devient `AGENTS.md`, source unique des
  instructions quel que soit l'outil, et `CLAUDE.md` ne fait plus que l'importer. Les procédures passent dans
  quatre skills sous `.agents/skills/` — `session`, `build`, `cloture`, `questions` —, chacun avec un
  `GOTCHA.md` qui recense les pièges rencontrés depuis le début du projet. `.claude/skills` est un lien vers ce
  dossier.
- Décisions : pas d'ADR, ce n'est pas une décision d'architecture du moteur. La règle « un contrôle échoue
  bruyamment », adoptée à la clôture de la phase 0, devient la règle non négociable n°7.
- Écarts et problèmes : la découverte des skills par Claude Code **à travers le lien** n'est pas vérifiable dans
  cette session, la liste étant chargée au démarrage. Sans conséquence si elle échoue : `AGENTS.md` donne le
  chemin de chaque skill.
- Prochaine étape : au début de la prochaine session, vérifier que les quatre skills apparaissent. Puis #38,
  capture Tracy.

## 2026-09-21 — M1.1 — Frame time, sanitizers et avertissements (#11)

- **Temps Donnovan : 0,31 h** (10 min déclarées, relectures de #50 et de #51 ; 0,31 h après réconciliation,
  voir la clôture ; estimé 0,75 h)
- Sessions Claude Code : 1 (la même que #10, PR découpée)
- Fait : `recordFrame` dans `core` (moyenne, minimum et maximum par période d'une seconde) et trois tests ;
  frame time dans le titre de la fenêtre ; `setWindowTitle` et son assertion ASCII ; preset `linux-asan`
  (AddressSanitizer, LeakSanitizer, UBSan, `-fno-sanitize-recover=all`) et son job CI, avec un garde-fou qui
  vérifie que le binaire est bien instrumenté ; `-Wall -Wextra -Werror` sur tout le code du moteur.
- Mesures :
  - titre lu par KWin, build ASan : `Levain - 0.002 ms (min 0.002, max 0.083) - 605107 images/s`
    (`gdbus call --session --dest org.kde.KWin --object-path /WindowsRunner --method org.kde.krunner1.Match Levain`) ;
  - temps passé masquée exclu du frame time : max **0,036 ms** 0,3 s après une minimisation de 2 s ;
    **contre-test**, la remise à l'heure retirée : max **2 314,870 ms** (même commande, pilotage par KWin) ;
  - sanitizers : 21 tests et 3 s de sandbox, zéro fuite et zéro comportement indéfini, en offscreen (comme la
    CI) et sous Wayland (`SDL_VIDEO_DRIVER=offscreen timeout --foreground --preserve-status -k 10 3 ./build/linux-asan/sandbox/levain_sandbox`) ;
  - garde-fou d'instrumentation : 49 symboles `__ubsan_handle` dans le binaire ASan, aucun `__asan_init` dans
    le binaire Debug (`nm … | grep`) ;
  - avertissements à l'activation : **5**, pas 4 comme annoncé — ma première mesure ne portait que sur le
    Debug. 4 `-Wmissing-designated-field-initializers` (un `WindowEvent` sans `pixelSize`), et en Release une
    variable qui ne servait qu'à une assertion (build des trois presets avec `-Wall -Wextra -Werror`).
- Écarts et problèmes :
  - **Première vraie trouvaille du sanitizer, et elle est chez SDL.** Sous X11, LeakSanitizer signalait une
    fuite dans `SDL_X11_SetWindowTitle` (`SDL_x11window.c:2300`). En remontant : en locale C, la conversion
    du titre pour l'ancienne propriété `WM_NAME` échoue sur « — », et SDL abandonne **sans rien dire** —
    sans libérer la conversion, et sans envoyer le titre UTF-8. KWin affichait toujours « Levain » : le frame
    time n'avait jamais atteint l'écran. Confirmé par l'expérience (titre ASCII : ni fuite, titre affiché) ;
    même code sur la branche `main` de SDL. Parade : titres ASCII, vérifiés par assertion.
  - **Faux positifs LeakSanitizer sous X11** : 50 052 octets en 913 allocations, la mémoire permanente de
    libX11 que SDL décharge par `dlclose` à la sortie. Précharger libX11 les fait disparaître sans masquer les
    vraies fuites : la fuite du titre restait signalée. Zéro sous Wayland et en offscreen.
  - clang-tidy 22 compte `optional::value()` comme un accès non vérifié, et ne voit pas le `REQUIRE` de
    doctest comme une garde : les tests passent par `value_or(FrameTimeSummary{})`, dont les zéros font
    échouer les `CHECK` si aucun résumé n'est rendu.
  - **`LEVAIN_ASSERT` en Release** passait de `((void)0)` à `((void)sizeof(static_cast<bool>(expression)))` :
    l'expression est compilée sans être évaluée. Une variable qui ne sert qu'à une assertion n'est plus
    « inutilisée », et une assertion qui ne compile plus casse aussi le Release. Reste un cas : une fonction
    interne qui n'apparaît que dans une assertion, que clang déclare inutile (`isAscii`, `[[maybe_unused]]`).
- Décisions (Donnovan, 21/09) : `linux-asan` devient un **check requis** pour fusionner sur `main` ;
  avertissements activés ; **rien n'est signalé en amont à SDL**, dont les mainteneurs n'acceptent pas les
  contributions d'IA. Les deux bugs restent documentés dans `engine/platform/README.md`.
- Prochaine étape : #38, capture Tracy. vcpkg n'ayant toujours pas Tracy 0.14, la voie suivante décidée en
  M0.3 est un port overlay en 0.14.1.

## 2026-09-21 — M1.1 — Fenêtre SDL3, boucle et événements (#10)

- **Temps Donnovan : 0,63 h** (20 min déclarées, 0,63 h après réconciliation avec le total de la journée, voir
  la clôture ; estimé 0,75 h)
- Sessions Claude Code : 1
- Fait : module `engine/platform` sur SDL3 3.4.12 (vcpkg, fonctionnalités `x11` et `wayland` seulement, sans
  ibus ni dbus) ; événements traduits vers nos types (`CloseRequested`, `Resized` en pixels, `Hidden`,
  `Shown`) ; boucle du sandbox qui dort quand la fenêtre est masquée ; script `tools/kwin-window-smoke.sh`
  qui pilote la vraie fenêtre à travers KWin ; sandbox lancé 3 s en CI, arrêté par SIGTERM.
- Mesures :
  - redimensionnement puis minimisation et restauration sous X11 : aucun plantage, tailles reçues
    640 × 332 et 1600 × 872 px pour des cadres de 640 × 360 et 1600 × 900 — la barre de titre prend 28 px
    (`tools/kwin-window-smoke.sh`) ;
  - temps CPU : **2 010 ms en 2 s visible, 0 ms minimisée** (même script) ;
  - arrêt propre sur SIGTERM, code 0, en Debug et en Release
    (`SDL_VIDEO_DRIVER=offscreen timeout --foreground --preserve-status -k 10 3 ./build/<preset>/sandbox/levain_sandbox`) ;
  - aucun en-tête SDL hors de `engine/platform/src/` (`grep -rn "include.*SDL" engine sandbox tests`) ;
  - SDL3 compilé par vcpkg en 22 s à froid (`time cmake --preset linux-debug`).
- Écarts et problèmes :
  - **Sous Wayland, la fenêtre n'apparaît pas.** Une surface Wayland n'est affichée qu'après son premier
    buffer, et le moteur ne présente encore rien : KWin ne la connaît pas, alors qu'elle s'affiche sous X11.
    M1.1 se teste donc sous XWayland (`SDL_VIDEO_DRIVER=x11`), et la minimisation sous Wayland est reportée à
    M1.2, ajoutée à l'issue #13.
  - **Assertion de SDL sur un double signal** (`SDL_quit.c:171`) : `timeout` envoie SIGTERM à l'enfant puis
    au groupe de processus ; le second signal tombe entre la remise à zéro d'un drapeau et l'assertion qui la
    vérifie. Le SDL de Debug a alors ouvert une boîte de dialogue zenity sur le bureau, et le programme a
    attendu jusqu'au SIGKILL. Parade : `timeout --foreground`, un seul signal.
  - Un processus lancé avec `&` depuis un shell non interactif hérite d'un SIGINT **ignoré**, et SDL respecte
    ce choix : c'est pourquoi la CI et le script arrêtent le sandbox par SIGTERM.
  - **CI rouge deux fois sur SDL3** : la liste de paquets `-dev` suggérée par le port vcpkg ne suffit pas.
    SDL refuse de se configurer s'il manque une extension X11 demandée, d'abord Xcursor, puis XTest. La liste
    vient maintenant des contrôles stricts de `cmake/sdlchecks.cmake` (huit extensions) et de la section
    Ubuntu du `README-linux` de SDL. Bon point : SDL échoue bruyamment au lieu de désactiver la fonction.
  - **PR découpée** : la version complète de M1.1 faisait environ 710 lignes, près du double de la règle
    n°2. Le frame time et les sanitizers (#11) suivent dans une seconde PR, déjà prête.
  - Tracy (#38) : vcpkg `master` est toujours en 0.13.1 au 21/09.
- Prochaine étape : PR de l'issue #11 — frame time dans le titre, ASan et UBSan en CI.

## 2026-09-20 — M0.3 et phase 0 — Clôture

- **Temps Donnovan : 5,0 h** (12h00–14h30 et 21h30–23h55, soit ~4,9 h, arrondi au quart d'heure sur le board)
- Définition de « terminé » (SPECS §9) : 5 critères sur 7 remplis — démo lançable sous Linux, mesures
  consignées, CI verte, README du module à jour, **étude E0 écrite**. Deux sans objet : pas de démo Windows
  (hors périmètre, ADR-0011), pas de binaire à publier.

### L'erreur de mesure, corrigée par Donnovan

**Je mesurais la mauvaise chose depuis le début du projet.** Je demandais « combien de temps sur la
**relecture** » et n'enregistrais que ça, alors que les « Heures Donnovan » de la ROADMAP comptent **tout**
l'engagement : pilotage, questions, décisions, relecture.

| | Estimé | Passé | Ratio | Conséquence |
|---|---:|---:|---:|---|
| Mesure erronée (relectures seules) | 6,0 h | 3,0 h | **0,50** | recalibrage déclenché, −30 % sur 63 h |
| **Mesure réelle** (Donnovan, 20/09) | 6,0 h | **5,0 h** | **0,83** | **dans la fourchette, rien à changer** |

Le ratio erroné aurait amputé la roadmap d'environ 19 h sans aucune raison. CLAUDE.md est corrigé : la question
de fin de session porte désormais sur le **temps total**, avec la raison écrite pour qu'elle ne redérive pas.

### Phase 0 — bilan

| Milestone | Estimé | Passé |
|---|---:|---:|
| M0.1 Dépôt, suivi et specs | 1,0 h | 1,25 h |
| M0.2 Squelette de build et CI | 1,5 h | 1,50 h |
| M0.4 Socle Rust | 1,0 h | 0,25 h |
| M0.5 Retour au C++ | 1,0 h | 0,75 h |
| M0.3 Core minimal | 1,5 h | 1,25 h |
| **Phase 0** | **6,0 h** | **5,0 h** — ratio **0,83** |

**Aucun recalibrage** : le ratio est dans la fourchette 0,8–1,25 de la ROADMAP. À réexaminer à la clôture de la
phase 1, premier échantillon de vrai code de rendu — la phase 0 était faite de specs, d'ADR et de
configuration, et ne prédit pas grand-chose.

### Trois pannes silencieuses, un même motif

La journée en a produit trois, toutes « vertes » pendant qu'elles ne faisaient rien :

1. **L'épinglage de LLVM 22 en CI** — inopérant depuis M0.2, trois milestones sur clang 18. Le workflow
   imprimait la version à chaque run ; personne ne l'a lue.
2. **La baseline vcpkg** — contournée en local par le `spdlog` d'Arch, sans un mot. Découverte parce que Tracy
   n'existe pas en paquet système.
3. **Tracy sans `TRACY_NO_EXIT`** — le programme se termine avant qu'un profileur ait pu se connecter, sans
   avertissement.

Règle inscrite : **un contrôle doit échouer bruyamment quand sa condition n'est pas réunie, jamais se contenter
de ne pas s'exécuter.** Deux garde-fous ajoutés en conséquence (vérification de la chaîne LLVM, refus de
configurer sans toolchain vcpkg).

### Numérotation des ADR

La ROADMAP pré-attribuait cinq numéros d'ADR ; l'aller-retour Rust en a consommé deux (0010, 0011) et toute la
suite avait glissé — M2.1 renvoyait à « ADR-0009 », devenu le style C++. **Les numéros pré-attribués sont
retirés** : un ADR prend son numéro au moment où on l'écrit. Règle ajoutée à la ROADMAP.

### Phase 2 détaillée

Le rituel de clôture de phase demande de détailler la phase N+2. **8 issues créées** (#40 à #47) pour 5,5 h,
conformes au découpage de la ROADMAP : stratégie de binding, caméra et meshes, instancing et timestamps,
textures et mipmaps, samplers, hot-reload Slang, repli sur erreur, étude E2.

- Prochaine étape : **phase 1**, M1.1 — fenêtre SDL3, boucle principale, frame time, sanitizers. Voir aussi
  l'issue #38 (capture Tracy reportée) et #32 déjà close.

## 2026-09-20 — M0.3 — Allocateurs, Tracy et étude E0 (issues #7, #8, #9)

- Temps Donnovan : à renseigner (relecture estimée 0,3 h)
- Sessions Claude Code : 1
- Fait : `LinearAllocator` et `PoolAllocator` avec 11 tests, benchmark, macros de profilage Tracy,
  instrumentation du sandbox, étude **E0 — Comment démarre un moteur**.
- **Benchmark des allocateurs** (Release, machine de référence, médiane de 5 exécutions de 100 000 allocations
  de 64 octets) :

  | Allocateur | ns / allocation | Rapport à `malloc` |
  |---|---:|---:|
  | `malloc` + `free` | 14,50 | 1,0× |
  | `LinearAllocator` | **1,32** | **11,0×** |
  | `PoolAllocator` (alloc + free) | **2,38** | **6,1×** |

  Commande : `cmake --preset linux-release && cmake --build --preset linux-release &&
  ./build/linux-release/tests/levain_bench`

- **Tracy, coût nul quand désactivé — vérifié, pas affirmé** :

  | | Symboles Tracy | Bibliothèques liées | Taille du sandbox |
  |---|---:|---:|---:|
  | Désactivé (défaut) | **0** | **0** | 7 607 232 o |
  | `-DLEVAIN_PROFILING=ON` | 1 025 | 1 | 9 422 024 o |

  Commandes : `nm -C <binaire> \| grep -ci tracy`, `ldd <binaire> \| grep -ci tracy`.

- **Un test a trouvé un vrai bug dans mon allocateur.** `LinearAllocator::allocate` alignait l'**offset** dans
  le tampon et non l'**adresse réelle** ; `make_unique<std::byte[]>` ne garantit que l'alignement par défaut
  (16 octets), donc toute demande supérieure rendait un pointeur mal aligné — silencieusement, puisque ça
  « marche » sur x86. Corrigé en alignant l'adresse. C'est exactement ce que le critère « tests d'alignement »
  de l'issue devait attraper.
- **clang-tidy a trouvé quatre défauts de plus** : deux conversions implicites `void**` → `void*` dans les
  `memcpy` de la liste des libres, une multiplication en `int` élargie en `size_t`, et une exception pouvant
  s'échapper du `main` du benchmark. Tous corrigés, aucun désactivé. Troisième passage de l'outil sur du code
  neuf, troisième récolte.
- **Défaut de ma démo, trouvé par Donnovan en la lançant** : 120 frames à 200 µs font ~24 ms, impossible d'y
  connecter un profileur à la main. La parade est `TRACY_NO_EXIT=1`, qui fait attendre le client jusqu'à ce que
  le profileur se connecte et ait tout reçu. Documenté dans CLAUDE.md. Sans elle, le programme se termine sans
  le moindre avertissement — encore une panne silencieuse.
- Écarts et problèmes : **le critère « capture d'écran Tracy » de l'issue #8 n'est pas rempli, et il est
  reporté à M1.1** (issue #38) plutôt que maquillé. Trois raisons cumulées :
  1. **Versions incompatibles.** Le client vient de vcpkg en **0.13.1** ; les binaires Linux du profileur ne
     commencent qu'à **0.14.0**, et Tracy refuse une connexion dont le protocole ne correspond pas. vcpkg ne
     connaît aucune version ≥ 0.14 (`versions/t-/tracy.json`), donc pas d'`override` possible. Compiler le
     profileur 0.13.1 par `tracy[gui-tools]` reste faisable, mais c'est une interface graphique complète à
     construire depuis les sources.
  2. Aucun profileur installé sur la machine, et pas de paquet Arch.
  3. **La capture n'aurait rien montré d'utile** : boucle factice de 120 frames dont l'essentiel est un `sleep`.
     À M1.1 il y aura une vraie boucle, et une capture dira enfin où part la frame.

  Ce qui est vérifié aujourd'hui : les symboles `__tracy_source_location` sont présents dans le binaire
  instrumenté, et le coût nul quand Tracy est désactivé est mesuré.
- Le sandbox a maintenant une **boucle simulée de 120 frames** : il fallait quelque chose à découper pour que
  `LEVAIN_PROFILE_FRAME` ait un sens. La vraie boucle arrive en M1.1.
- **Deuxième contrôle silencieusement inopérant de la session**, trouvé par Donnovan en lançant la commande
  Tracy que je lui avais donnée. `VCPKG_ROOT` n'était pas exportée dans son shell, donc la toolchain vcpkg
  n'était pas chargée — et **CMake a trouvé le `spdlog` d'Arch dans `/usr/lib/cmake/spdlog` et continué sans
  rien dire**, contournant la baseline figée de l'ADR-0007. Le build n'a échoué que sur Tracy, qui n'existe pas
  en paquet système. Une dépendance de moins et personne ne s'apercevait de rien.
  Garde-fou posé : le `CMakeLists.txt` racine refuse de se configurer si `VCPKG_TOOLCHAIN` n'est pas défini,
  avec un message qui dit quoi faire. Vérifié dans les deux sens.
- **Le motif de la session** : deux vérifications ont passé pendant des semaines en ne faisant rien — l'épinglage
  de LLVM en CI, et la baseline vcpkg en local. Les deux étaient « vertes ». À retenir : **un contrôle doit
  échouer bruyamment quand sa condition n'est pas réunie, jamais se contenter de ne pas s'exécuter.**
- Prochaine étape : clôture de M0.3 et de la phase 0 — ratio, recalibrage de la roadmap, et détail des issues
  de la phase 2.

## 2026-09-20 — M0.3 — Logs, assertions et gestion d'erreurs (issue #6)

- Temps Donnovan : à renseigner (relecture estimée 0,25 h)
- Sessions Claude Code : 1
- Fait : `log.hpp` (catégories et niveaux, spdlog), `assert.hpp` (`LEVAIN_ASSERT`, `LEVAIN_VERIFY`),
  `error.hpp` (`Result<T>` = `std::expected<T, Error>`), **ADR-0008**, 7 nouveaux tests, README du module
  `core` mis à jour.
- **ADR-0008 : le débat « exceptions ou codes de retour » est mal posé.** Il y a deux sortes d'échecs et elles
  n'appellent pas la même réponse : un **bug du moteur** s'arrête au plus près de la faute (`LEVAIN_ASSERT`),
  un **échec de l'environnement** se renvoie (`Result<T>`). C'est le cœur de l'ADR.
- Décision inattendue : **`-fno-exceptions` n'est pas activé**, alors que c'est la pratique courante des
  moteurs. Raison trouvée dans la doc Godot : avec les exceptions désactivées, le `throw` de libstdc++ se
  replie sur `__builtin_trap()` — arrêt brutal sans message. Unreal et Godot peuvent se le permettre parce
  qu'ils ont remplacé la STL par leurs propres conteneurs ; nous l'utilisons pleinement.
- Mesures :

  | Critère de l'issue | Résultat | Commande |
  |---|---|---|
  | Une assertion affiche fichier, ligne et message, puis s'arrête dans le débogueur | **oui**, et code de sortie **133** (SIGTRAP) | programme de démonstration lié à `levain_core` |
  | Tests | **9 verts en Debug et en Release** | `ctest --test-dir build/linux-{debug,release}` |
  | clang-tidy | **0 finding** | `clang-tidy -p build/linux-debug --warnings-as-errors='*'` |

  Sortie de l'assertion violée :
  ```
  [critical] [assert] assertion violée : frameCount >= 0
    message  : le compteur de frames ne peut pas être négatif
    assert_demo.cpp:5 (int main())
  ```

- **clang-tidy a encore trouvé deux vrais points** dès la première exécution sur ce code : `ErrorCode` et
  `LogLevel` utilisaient `int` comme type sous-jacent là où `std::uint8_t` suffit (`performance-enum-size`).
  Corrigé, pas désactivé. C'est la deuxième fois que l'outil paye dès son premier passage sur du code neuf.
- Conformité à l'ADR-0011 (forme du code) : tout est en **fonctions libres avec les dépendances dans la
  signature**, et **chaque piège porte son nom** — `isLogEnabled` avant le formatage pour qu'un `Trace` dans
  une boucle de rendu ne paie pas son `std::format`, `LEVAIN_VERIFY` pour l'expression à effet qui ne doit pas
  disparaître en Release. Les trois règles ont tenu sur du vrai code.
- Écarts et problèmes : `isLogEnabled` fait une **recherche par chaîne à chaque appel**. Noté dans le code :
  si une capture Tracy (issue #8) montre le log dans le profil, la réponse sera un handle de catégorie obtenu
  une fois, pas une optimisation de la table.
- Prochaine étape : issues #7 (allocateurs et benchmarks), #8 (Tracy) et #9 (étude E0), dans une seconde PR.

## 2026-09-20 — M0.5 — Clôture du milestone

- Temps Donnovan : inclus dans les 0,25 h de la relecture de la PR #33
- **Cumul de la phase 0 à ce stade** : **2,75 h passées pour 3,75 h estimées**, soit un **ratio de 0,73** sur
  quatre milestones terminés (M0.1, M0.2, M0.4, M0.5). Reste M0.3, estimé 1,5 h.
  **L'aller-retour par Rust aura coûté 0,5 h de Donnovan au total** — deux milestones, quatre relectures d'un
  quart d'heure. C'est le prix d'avoir tranché la question du langage définitivement, à 107 lignes de code.
- Définition de « terminé » (SPECS §9) : 4 critères sur 7 s'appliquent et sont remplis (démo lançable sous
  Linux, mesures consignées, CI verte, docs à jour). Trois restent sans objet à ce stade : pas de démo Windows
  (hors périmètre, ADR-0011), pas d'étude de phase due en M0.5, pas de binaire à publier.
- Fait : ADR-0011 accepté, tag `m0.5`, release avec les mesures, milestone fermé. Issue #32 fermée — elle
  demandait de réécrire les issues de phase 1 pour la pile Rust, sans objet depuis le retour au C++.
- **Question archivée en Q&R** : « Clang existe aussi sous Windows, pourquoi MSVC ? » Réponse contre-intuitive —
  clang-cl ne règle **aucun** des deux bugs de M0.2 (il reproduit volontairement le piège `__cplusplus` et
  consomme la STL de Microsoft), mais il donnerait **exactement C++23 sur les deux plateformes** au lieu du
  sur-ensemble `/std:c++latest`. Noté dans l'ADR-0011 comme première option à évaluer quand Windows reviendra.
- Observation sur la journée : trois décisions prises par défaut ont été attrapées par Donnovan et transformées
  en choix argumentés — la forme du code (variante C), le périmètre Windows, et le compilateur Windows. Aucune
  n'était signalée comme incertaine dans les ADR d'origine. **À faire systématiquement : marquer dans un ADR ce
  qui est un choix raisonné et ce qui est une convention reprise sans examen.**
- Prochaine étape : M0.3 — logs, assertions, ADR-0008 (gestion d'erreurs), allocateurs, Tracy, étude E0.

## 2026-09-20 — M0.5 — Retour au C++ (ADR-0011)

- Temps Donnovan : **0,25 h** pour 1,0 h estimée (ratio 0,25) — relecture de la PR #33. Issue #35 créée
  rétroactivement pour que le board porte ce temps : le travail avait été décidé en conversation, sans issue.
- Sessions Claude Code : 1
- Contexte : Donnovan revient sur la décision Rust. Sa thèse : ce qu'il trouvait plus lisible venait de la
  **simplicité du langage**, et du C++ écrit en exploitant ses atouts devrait se lire aussi bien. Il précise
  aussi que **le projet est sous Linux** et que Windows n'est pas un sujet pour l'instant.
- **Deux biais reconnus dans l'ADR-0010**, et c'est ce qui rend le revirement fondé :
  1. **La manche 1 comparait flecs à bevy_ecs, pas C++ à Rust.** La propriété que Donnovan a aimée — les
     dépendances dans la signature — appartient à bevy_ecs, pas à Rust. Mon C++ était handicapé par la glu
     flecs (lambda `[](flecs::iter&, size_t, …)`, `it.world().get<>()` caché dans le corps). Une fonction libre
     a exactement la même propriété. **Je n'ai pas écrit le meilleur C++ possible.**
  2. **La manche 3 facturait au C++ un Windows dont le projet n'a pas besoin.** Et surtout : **les deux seuls
     bugs de M0.2 étaient des bugs Windows** (`/Zc:__cplusplus`, `<ostream>` non inclus en cascade par la STL
     de Microsoft). En périmètre Linux, ni l'un ni l'autre n'existe.
- Décisions (ADR-0011, remplace ADR-0010) : retour au C++23 ; **Linux d'abord**, presets et CI Windows retirés,
  M1.4 différé (et non plus supprimé) ; **forme du code fixée** — variante C choisie par Donnovan sur lecture
  de trois variantes : fonctions libres avec toutes les dépendances dans la signature, glu ECS confinée à une
  ligne, chaque piège portant son nom (`normalizeOrZero`, `clampPitch`, `horizontalBasisFrom`).
- Méthode : le revert n'a **pas** annulé `docs/JOURNAL.md` ni l'ADR-0010, restaurés depuis `main`. Un journal
  et une décision sont de l'historique, ils ne se revertent pas.
- Mesures :

  | Critère | Résultat | Commande |
  |---|---|---|
  | Build et sandbox | `Levain 0.1.0 — clang 22.1.8 — __cplusplus 202302` | `cmake --build --preset linux-debug` |
  | Tests | **2 verts** | `ctest --test-dir build/linux-debug` |
  | Format | conforme | `clang-format --dry-run --Werror` |
  | clang-tidy | **0 finding** | `clang-tidy -p build/linux-debug --warnings-as-errors='*'` |
  | Infrastructure | **347 lignes** (contre 435 en v0.3) | `wc -l` sur les 9 fichiers |

- **Correction d'une estimation que j'avais donnée pour une mesure** : j'annonçais « environ 300 lignes » avant
  d'avoir mesuré ; le vrai chiffre est **347**. La moitié des 435 lignes de la v0.3 était du format et du lint,
  indépendants de la plateforme. Ce que le périmètre Linux fait disparaître n'est pas du volume mais **la part
  qui causait les pannes**.
- Écarts et problèmes : les jobs Windows n'existent plus, donc les checks requis par la protection de `main`
  (`linux-debug`, `linux-release`) restent valides sans modification — les noms de presets n'ont pas changé.
  L'issue #32 (réécrire les issues de phase 1) devient sans objet et sera refermée.
- Bilan des deux allers-retours : **le détour par Rust a produit quelque chose**. Sans la comparaison, la
  variante C n'aurait jamais été écrite, et l'ADR-0009 seul n'avait pas suffi à la produire. Règle retenue pour
  la suite : **avant de conclure qu'une alternative est meilleure, vérifier qu'on a écrit la meilleure version
  de ce qu'on compare.**
- Prochaine étape : validation de l'ADR-0011, clôture de M0.5, puis M0.3 — logs, assertions, ADR-0008,
  allocateurs, Tracy, étude E0.

## 2026-09-20 — M0.4 — Socle Rust (issues #27 à #30)

- Temps Donnovan : **0,25 h** pour 1,0 h estimée (ratio 0,25) — relecture de la PR #31. Les quatre issues ayant
  été livrées en une seule PR, le temps est porté sur l'issue #27 au board plutôt que réparti.
- Sessions Claude Code : 1
- Fait : migration complète vers Rust. Workspace cargo, crates `levain-core` et `levain-sandbox`, CI réécrite,
  documentation répercutée, C++ supprimé de l'arbre de travail.
- Mesures :

  | Critère | Résultat | Commande |
  |---|---|---|
  | Le sandbox tourne | `Levain 0.1.0 — linux/x86_64 — rust edition 2024` | `cargo run -p levain-sandbox` |
  | Tests | **2 tests verts** | `cargo test --workspace` |
  | Un code mal formaté fait échouer la CI | code de sortie **1** | `cargo fmt --all --check` |
  | Un défaut clippy fait échouer la CI | **erreur** sur une fonction jamais utilisée | `cargo clippy --workspace --all-targets -- -D warnings` |
  | Infrastructure de build et CI | **136 lignes** contre 435 en C++ | `wc -l` sur Cargo.toml ×3 et ci.yml |

- Durées de CI, premier run, **les 4 jobs verts du premier coup** :

  | Job | Rust (à froid) | C++ à froid | C++ à chaud |
  |---|---:|---:|---:|
  | `linux-debug` | **16 s** | 85 s | 24 s |
  | `linux-release` | **8 s** | 125 s | 24 s |
  | `windows-debug` | **38 s** | 233 s | 52 s |
  | `windows-release` | **34 s** | 288 s | 91 s |
  | **Total** | **96 s** | 731 s | 191 s |

  **Réserve importante : ce n'est pas une comparaison équitable.** Le workspace Rust n'a aujourd'hui
  **aucune dépendance externe**, là où le build C++ compilait nvrhi, flecs et vulkan-headers. L'écart mesuré
  reflète surtout ça. La comparaison honnête viendra quand wgpu et bevy_ecs seront réellement ajoutés (M1.2 et
  M3.1) — et wgpu est un gros crate. Ce que ces chiffres établissent vraiment, c'est que **le chemin Windows a
  fonctionné du premier coup**, sans `vcvars`, sans `vswhere` et sans divergence de bibliothèque standard, là
  où M0.2 avait demandé deux correctifs.

- Outillage : `rustup` installé en mode utilisateur dans `~/.cargo` (pas de sudo), Rust **1.98.1**, avec
  rustfmt et clippy.
- Décisions prises en chemin :
  - **Noms de jobs de CI conservés à l'identique** (`linux-debug`, `linux-release`, `windows-debug`,
    `windows-release`) alors que le profil cargo s'appelle `dev`. Les renommer aurait rendu `main`
    infusionnable : ce sont les checks requis par la protection de branche. Le piège était noté dans l'issue #28,
    et le contourner coûte moins cher que de reconfigurer la protection.
  - **`clippy::all` seulement, pas `pedantic`.** Même raisonnement que pour clang-tidy en M0.2 : on active ce
    qui attrape de vrais défauts. `pedantic` pousse vers `must_use` partout, ce que l'ADR-0009 a écarté sous le
    nom de « décoration maximale ».
  - **Le test de version du C++ n'a pas été porté tel quel.** Il vérifiait que CMake injectait correctement
    `LEVAIN_VERSION` — un câblage qui n'existe plus, `env!("CARGO_PKG_VERSION")` étant automatique. Remplacé par
    un test qui vérifie que la version est un semver à trois composants, ce qui attrape un `Cargo.toml` malformé.
  - **Pas de dossier `tests/` à la racine** : les tests unitaires vivent dans le crate (`#[cfg(test)] mod tests`).
    SPECS §7 mis à jour en conséquence.
- **Erreur corrigée dans l'ADR-0010** : l'en-tête ne listait pas l'ADR-0003 (SDL3) parmi les ADR remplacés, alors
  que la décision remplace bien SDL3 par winit. Relevé en appliquant l'ADR. L'ADR-0003 est passé à « remplacé ».
- Écarts et problèmes : **les issues de la phase 1 nomment encore NVRHI, SDL3 et Slang** (#10, #12 à #17). Elles
  ne sont pas réécrites ici — le périmètre de l'issue #30 s'arrêtait à SPECS, ROADMAP, CLAUDE.md et aux statuts
  d'ADR — mais elles devront l'être **avant d'attaquer M1.1**.
- Roadmap : **M1.4 supprimé**, **M0.4 ajouté**, total inchangé à 68 h. Phase 0 passe de 4,0 à 5,0 h, phase 1 de
  5,5 à 4,5 h.
- Prochaine étape : clôture de M0.4, puis M0.3 — logs, assertions, ADR-0008, allocateurs, Tracy, étude E0.

## 2026-09-20 — M0.2 — Réévaluation du langage : passage à Rust (ADR-0010)

- Temps Donnovan : à renseigner (trois manches de lecture de code + relecture de l'ADR, estimée 0,3 h)
- Sessions Claude Code : 1
- Contexte : Donnovan demande à la clôture de M0.2 pourquoi ne pas passer à Rust, puisqu'il n'écrit pas le code
  et qu'il fait du C++ toute la semaine. L'ADR-0001 prévoyait explicitement cette réévaluation.
- Méthode : trois manches de comparaison sur du code réel plutôt qu'une discussion de principes, comme pour
  l'ADR-0009.
  1. **Transform + déplacement FPS** → Donnovan trouve le **Rust plus lisible sans en avoir jamais lu**, et le
     C++ plus verbeux alors qu'il en fait tous les jours.
  2. **Propagation hiérarchique des transforms** → manche **favorable au C++** : le `cascade()` de flecs tient
     en 10 lignes, la version Rust sûre en demande 20. Bevy utilise `unsafe` dans la sienne (bevy#4697).
  3. **Build et CI** → mesuré sur le dépôt réel : **435 lignes d'infrastructure C++ contre 94 en Rust**, pour
     107 lignes de moteur. Commande : `wc -l` sur ci.yml, CMakePresets.json, les CMakeLists, vcpkg.json,
     .clang-format et .clang-tidy, contre l'équivalent cargo écrit et compté.
- Décision proposée : **ADR-0010**, passage à Rust edition 2024. wgpu, bevy_ecs, winit, glam, rapier3d, gltf,
  WGSL, egui, tracing, tracy-client, kira. Remplace les ADR 0001, 0002, 0004, 0005, 0007 et la moitié nommage
  de l'ADR-0009.
- `bevy_ecs` et non `flecs_ecs` : le binding Rust de flecs existe et couvre les hiérarchies, mais il est
  auto-déclaré **alpha**, maintenu par une personne, à 11 359 téléchargements, et son `World` est
  `!Send`/`!Sync`. On ne pose pas le cœur du moteur dessus.
- Conséquence sur la roadmap : **M1.4 (backend Direct3D 12) disparaît** — wgpu choisit son backend seul.
  1,0 h de budget et une session rendues. Nouveau milestone **M0.4 — Socle Rust** pour refaire l'équivalent
  de M0.2.
- Correction d'une erreur de l'ADR-0001 : « le borrow checker résiste aux graphes d'objets » était vrai en
  général et hors sujet ici, la réponse Rust à ce problème étant l'ECS, qu'on avait déjà choisi.
- Écarts et problèmes : on perd le parcours de lecture Donut, sur lequel l'ADR-0002 et CLAUDE.md étaient
  bâtis, et l'objectif « comprendre Unreal, Unity, Godot » se paie plus cher puisqu'ils sont tous en C++.
  Assumé et écrit dans l'ADR.
- Prochaine étape : validation de l'ADR-0010 par Donnovan. **Rien n'est migré tant qu'il n'est pas accepté.**
  Ensuite : ROADMAP et SPECS mis à jour, milestone M0.4 créé, puis socle Rust.

## 2026-09-20 — M0.2 — clang-tidy, doctest et protection de main (issue #5)

- Temps Donnovan : à renseigner (relecture estimée 0,3 h)
- Sessions Claude Code : 1
- Fait : `.clang-tidy` (nommage de l'ADR-0009 + bugprone, performance, quelques modernize), doctest par vcpkg
  avec 2 cas découverts individuellement, `ctest` branché, contrôle de format et analyse statique en CI,
  protection de `main`.
- Mesures — les trois critères de l'issue :

  | Critère | Résultat | Commande |
  |---|---|---|
  | Un code mal formaté fait échouer la CI | code de sortie **123** sur format invalide, **0** sinon | `find … \| xargs clang-format --dry-run --Werror` |
  | `ctest` lance au moins un test | **2 tests**, verts | `ctest --test-dir build/linux-debug` |
  | Push direct sur `main` refusé | **refusé** (voir plus bas) | `git push origin main` |

  Le test sait échouer : `LEVAIN_EXPECTED_VERSION` forcé à `9.9.9` → `50% tests passed, 1 tests failed`.

- Durées de CI avec LLVM 22 et les tests : `linux-debug` 69 s, `linux-release` 56 s, `windows-debug` 289 s,
  `windows-release` 278 s. L'installation de LLVM 22 coûte une trentaine de secondes aux jobs Linux par rapport
  au run précédent (24 s), pour la garantie que clang-format et clang-tidy sont les mêmes qu'en local.

- **Deux vraies trouvailles, aucune stylistique.**

  1. **clang-tidy, première exécution** : `std::print` peut lever et `main` laissait l'exception s'échapper, ce
     qui appelle `std::terminate`. Corrigé dans `main`, pas désactivé (règle n°4). La politique générale reste
     l'affaire de l'ADR-0008 en M0.3.
  2. **CI Windows** : `version_test.cpp` ne compilait pas sous MSVC alors que Linux était vert. Pour afficher la
     valeur d'un `CHECK` qui échoue, doctest instancie `operator<<` vers un `ostream` ; la STL de Microsoft
     déclare cet opérateur pour `std::string_view` **sans inclure `<ostream>` en cascade**, là où libstdc++ le
     fait. Réglé par un `#include <ostream>`. **Première divergence de plateforme du projet**, sur 20 lignes de
     test, et détectable uniquement par le job Windows — celui qu'on avait gardé non bloquant « au cas où ».

- Correction d'une erreur d'analyse de ma part : j'avais justifié Clang 18 en CI par « la CI est plus
  conservatrice ». C'est faux — Clang 18 n'est pas plus strict, il est moins complet, et surtout clang-format 18
  et 22 ne produisent pas la même sortie. Un contrôle de format en 18 aurait rejeté des fichiers corrects
  formatés en 22. LLVM 22 installé en CI via `apt.llvm.org`, `LLVM_VERSION` dans le workflow à garder égal à la
  machine de référence (SPECS §10).

- Protection de `main` : PR obligatoire, `linux-debug` et `linux-release` requis, force-push et suppression
  interdits, **`enforce_admins` activé**. Les jobs Windows sont volontairement **hors des checks requis** :
  ils sont `continue-on-error`, les inscrire annulerait ce compromis. À ajouter le jour où ils deviendront
  bloquants.

- Écarts et problèmes : `enforce_admins: true` s'applique aussi à Donnovan. Pour lever la protection en cas de
  besoin : `gh api -X DELETE repos/PhantomDO/Levain/branches/main/protection`. Conséquence pour les sessions
  suivantes : **plus aucun push direct sur `main`**, y compris pour une entrée de journal.

- `bugprone-easily-swappable-parameters` est le seul check écarté : il signalerait
  `allocate(size_t size, size_t alignment)` et à peu près toute l'API d'un moteur.

- Prochaine étape : clôture de M0.2, puis M0.3 — logs, assertions, ADR-0008, allocateurs, Tracy, étude E0.

## 2026-09-20 — M0.2 — CI Windows + Linux (issue #4)

- Temps Donnovan : à renseigner (relecture estimée 0,25 h)
- Sessions Claude Code : 1
- Fait : `.github/workflows/ci.yml` — matrice de 4 jobs (Debug et Release × `ubuntu-latest` et
  `windows-latest`), Ninja partout, vcpkg cloné au tag `2026.07.29` et cache binaire via `actions/cache`.
  Jobs Windows en `continue-on-error`. **Les 4 jobs sont verts au premier run.**
- Mesures — durées de CI, à froid puis à chaud (cache vcpkg peuplé) :

  | Job | À froid | À chaud | Gain |
  |---|---:|---:|---:|
  | `linux-debug` | 85 s | **24 s** | −72 % |
  | `linux-release` | 125 s | **24 s** | −81 % |
  | `windows-debug` | 233 s | **52 s** | −78 % |
  | `windows-release` | 288 s | **91 s** | −68 % |
  | **Total** | **731 s** | **191 s** | **−74 %** |

  Commande : `gh api repos/PhantomDO/Levain/actions/runs/<id>/jobs`, différence entre `started_at` et
  `completed_at`. Runs `35518846705` (froid) et `35519196360` (chaud).

  Windows coûte 2 à 3 fois plus cher que Linux, à froid comme à chaud.

- **Confirmation empirique de l'ADR-0001.** Le même sandbox, mêmes sources, annonce :
  `Linux / Clang 18.1.3 / -std=c++23 → __cplusplus 202302` et
  `Windows / MSVC 19.51.36256 / /std:c++latest → __cplusplus 202400`.
  `202400` n'est la valeur d'aucune norme publiée : c'est un mode brouillon post-C++23. Le raisonnement de
  l'ADR-0001 est donc vérifié par la mesure, et le `-pedantic-errors` côté Linux n'est pas une précaution
  théorique.
- Écarts et problèmes :
  - **Erreur de ma part** : le premier run annonçait `__cplusplus 199711` sous Windows. MSVC épingle cette macro
    à la valeur de C++98 sauf si on passe `/Zc:__cplusplus`. La doc Microsoft que j'avais lue en écrivant
    l'ADR-0001 le dit explicitement ; je ne l'avais pas appliqué. Corrigé (`19b5fbc`), avec le constat écrit
    dans le commentaire du `CMakeLists.txt`.
  - **La CI Linux tourne sur Clang 18.1.3, la machine de référence sur Clang 22.1.8** — quatre versions
    majeures d'écart. L'écart va dans le sens le moins dangereux (la CI est plus conservatrice que la machine
    de dev), mais du code qui compile localement peut casser en CI. Laissé tel quel faute de cas concret ;
    à rouvrir si ça mord. Alternative : installer Clang 22 via `apt.llvm.org`, ~20 s par job.
  - **Pas de `ctest`** : aucun test n'existe encore. Test de fumée (lancement du sandbox) en attendant
    doctest, issue #5.
  - **`VCPKG_TAG` et `builtin-baseline` ne sont pas couplés automatiquement** : deux valeurs écrites à la main
    dans deux fichiers. Cohérentes aujourd'hui, vérifiées manuellement.
- Prochaine étape : issue #5 — clang-tidy (nommage de l'ADR-0009), doctest et `ctest`, vérification du format
  en CI, protection de `main`.

## 2026-09-20 — M0.2 — Norme de style C++ (ADR-0009)

- Temps Donnovan : 0,25 h de relecture de la PR #21 (portée au board), plus le choix de style
- Sessions Claude Code : 1
- Contexte : Donnovan demande une norme d'écriture fixée pour tout le projet. Le C++ propose plusieurs
  conventions incompatibles et il relit tout avec 1 à 2 h par semaine : sans règle, chaque fichier dérive et la
  relecture coûte de l'attention pour rien. Le dépôt comptait 27 lignes de code — aucune conversion à faire.
- Méthode : trois variantes complètes du même allocateur linéaire (le vrai, issue #7) soumises en lecture
  plutôt qu'en discussion. Choix fait sur le code, pas sur des principes.
- Décisions (ADR-0009) : **variante C** — types `PascalCase`, fonctions et variables `camelCase`, membres
  `m_` ; Allman, 4 espaces, 100 colonnes ; décoration **modérée** (`[[nodiscard]]` seulement quand ignorer le
  retour est un bug, `noexcept` seulement quand c'est garanti) ; commentaires **en français**.
- SPECS §8 amendé : les commentaires de code passent officiellement en français. J'étais déjà en infraction
  dans la PR #21 sans l'avoir signalé — corrigé.
- Mesures : `.clang-format` appliqué aux 3 fichiers existants, build et exécution vérifiés après reformatage
  (`clang-format -i` puis `cmake --build --preset linux-debug`), puis `clang-format --dry-run --Werror` passe
  sur les 3 fichiers.
- Écarts et problèmes : clang-format **ne vérifie pas le nommage** — c'est clang-tidy
  (`readability-identifier-naming`) qui s'en chargera dans l'issue #5. Tant que ce n'est pas en place, la moitié
  nommage de l'ADR-0009 repose sur ma discipline, pas sur l'outil.
- Décision de périmètre : la CI Windows de l'issue #4 sera **non bloquante** au début. Donnovan n'a pas de
  machine Windows ; les runners GitHub n'en demandent pas non plus, et c'est le seul endroit qui vérifiera le
  pari `/std:c++latest` de l'ADR-0001. Job gardé, `continue-on-error`, rendu bloquant quand une machine sera
  disponible.
- Prochaine étape : issue #4 (CI), puis #5 (clang-tidy, doctest, protection de `main`).

## 2026-09-20 — M0.2 — Squelette de build (issue #3)

- Temps Donnovan : à renseigner (relecture estimée 0,3 h)
- Sessions Claude Code : 1
- Fait : `CMakeLists.txt` racine, `CMakePresets.json` (4 presets, Ninja), `vcpkg.json` avec baseline figée sur la
  release vcpkg `2026.07.29` (`9e593bb…`), bibliothèque `levain_core` et exécutable `levain_sandbox`,
  `engine/core/README.md`, section « Commandes de build » de CLAUDE.md complétée.
- Mesures :
  - Build complet depuis zéro (Clang 22.1.8, Debug, sans vcpkg) → **4 étapes Ninja**, sandbox lancé :
    `Levain 0.1.0 — clang 22.1.8 — __cplusplus 202302`.
    Commande : `cmake -S . -B <dir> -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++`
  - Garde-fou C++23 vérifié **dans le vrai build** : `compile_commands.json` contient bien `-std=c++23`
    (et non `gnu++23`) et `-pedantic-errors` ; une indexation de paquets C++26 insérée dans `main.cpp` fait
    échouer la compilation avec `error: pack indexing is a C++2c extension`.
  - `CMAKE_CXX_SCAN_FOR_MODULES OFF` : le build passe de **8 à 4 étapes** Ninja. CMake activait le scan de
    modules C++20 par défaut alors que l'ADR-0001 dit « sans modules » — deux étapes par fichier pour un
    résultat toujours vide.
- Décisions : pas d'ADR, rien de structurant. Deux choix signalés en PR (arborescence partielle, dépendances
  déclarées non liées).
- Écart levé le jour même : `zip` installé par Donnovan, vcpkg bootstrappé
  (`2026-07-27-98d7cb0c`) dans `~/vcpkg`. Le critère **« build depuis un clone propre en deux commandes » est
  vérifié sur Linux**, sur un vrai `git clone` du dépôt public :

  | Mesure | Valeur | Détail |
  |---|---:|---|
  | `cmake --preset linux-debug` **à froid** | **25 s** | dont 23 s de vcpkg compilant `nvrhi` (2026-02-26), `flecs` et `vulkan-headers` depuis les sources |
  | `cmake --build --preset linux-debug` | **1 s** | 4 étapes Ninja |
  | `cmake --preset linux-debug` **à chaud** (2ᵉ clone) | **1 s** | vcpkg résout en **16,5 ms** depuis le cache binaire |
  | Taille du cache binaire vcpkg | **19 Mo** | `~/.cache/vcpkg` |

  Commandes : `git clone https://github.com/PhantomDO/Levain.git <dir> && cd <dir> && VCPKG_ROOT=~/vcpkg
  cmake --preset linux-debug && VCPKG_ROOT=~/vcpkg cmake --build --preset linux-debug`

  Sortie du sandbox : `Levain 0.1.0 — clang 22.1.8 — __cplusplus 202302`. `compile_commands.json` : 2 entrées,
  flags `-std=c++23 -pedantic-errors`.

  **Conséquence pour l'issue #4** : 19 Mo de cache pour un gain de 23 s à chaque exécution. Le cache binaire
  vcpkg vaut clairement le coup en CI, et il tient largement dans les quotas du cache GitHub Actions.
- Note pour M1.2 : le port vcpkg de `nvrhi` indique qu'il faut lier `nvrhi` **et** `nvrhi_vk` sous Linux
  (build statique), `nvrhi` seul en build partagé. Les cibles `nvrhi_d3d11`/`nvrhi_d3d12` n'existent que sous
  Windows.
- Prochaine étape : issue #4 (CI Windows + Linux avec cache vcpkg), puis #5 (clang-format, clang-tidy, doctest,
  protection de `main`). Reste non vérifié : le chemin **Windows**, qui n'existe que sur le papier tant que la CI
  n'a pas tourné — c'est l'objet de #4.

## 2026-09-20 — M0.1 — Clôture du milestone

- Temps Donnovan : 1,25 h (estimé 1,0 h — **ratio 1,25**)
- Sessions Claude Code : 1
- Définition de « terminé » (SPECS §9) : 4 critères sur 7 s'appliquent et sont remplis (mesures consignées,
  board à jour, étude E1 écrite d'avance, docs à jour). Trois ne s'appliquent pas encore, faute de code :
  démo `sandbox/`, CI verte, binaires de Release.
- Fait : tag `m0.1` posé et poussé. **Pas de GitHub Release** — §9 la prévoit pour livrer les binaires et les
  mesures de la démo, il n'y en a aucun à ce stade. Le premier Release sera celui de M0.2, qui aura une CI.
  Milestone n°1 fermé.
- Écarts et problèmes : le dépassement de 0,25 h vient entièrement de l'amendement C++23, hors estimation
  initiale. À surveiller sur M0.2 et M0.3 : si le ratio 1,25 se confirme, la roadmap passe de 68 h à ~85 h et il
  faudra recalibrer (ROADMAP, section « Recalibrage »).
- Prochaine étape : **M0.2 — Squelette de build et CI** (3 issues, 1,5 h estimée). Deux acquis de M0.1 à
  reprendre : `cxx_std_23` avec `-pedantic-errors` sur le job Linux comme garde-fou de conformité (ADR-0001), et
  la préférence GPU discret à prévoir pour M1.2 (SPECS §10).

## 2026-09-20 — M0.1 — Le moteur s'appelle Levain

- Temps Donnovan : inclus dans l'heure de M0.1
- Sessions Claude Code : 1
- Fait : nom choisi, **Levain**. Dépôt renommé `PhantomDO/GameEngine` → `PhantomDO/Levain` (GitHub redirige
  l'ancienne URL), board renommé « Levain — Roadmap », SPECS en v0.3, README, CLAUDE.md et SETUP mis à jour.
  Namespace racine `levain`, cibles CMake préfixées `levain_`.
- Décisions : le namespace français est une **exception assumée** à la règle « identifiants en anglais »
  (SPECS §8) — nom propre, comme Godot. Notée dans les conventions plutôt que subie.
- Écarts et problèmes : le dossier local est encore `~/Projects/GameEngine`. À renommer entre deux sessions
  (`mv ~/Projects/GameEngine ~/Projects/Levain`), pas pendant, pour ne pas casser la session en cours.
- Prochaine étape : M0.1 close. M0.2 — arborescence, presets CMake avec `cxx_std_23`, vcpkg en manifeste, CI
  avec `-pedantic-errors` sur le job Linux.

## 2026-09-20 — M0.1 — Passage à C++23 (ADR-0001 amendé)

- Temps Donnovan : 1,0 h (estimé 1,0 h pour M0.1 — ratio 1,0), porté sur le board (0,75 h sur l'issue #1,
  0,25 h sur l'issue #2)
- Sessions Claude Code : 1
- Contexte : Donnovan valide SPECS, ROADMAP et les ADR, et demande de passer à C++23 si la norme est stable.
  La v1 de l'ADR-0001 prévoyait exactement cette réévaluation.
- Mesures :
  - Sonde de macros de test de fonctionnalité, `-std=c++23` : Clang 22.1.8 et GCC 16.2.1 (libstdc++ 16) →
    `__cplusplus = 202302`, 19 fonctionnalités C++23 sur 19 présentes.
  - Garde-fou conformité : `clang++ -std=c++23 -pedantic-errors` et `g++ -std=c++23 -pedantic-errors` refusent
    bien l'indexation de paquets C++26 (`P2662`), acceptée en `-std=c++26`.
- Décisions : ADR-0001 amendé, C++20 → **C++23**, fichier renommé `0001-langage-cpp23.md`. Sans modules,
  inchangé.
- Écarts et problèmes : **MSVC n'a pas de `/std:c++23`**, ni en VS 2022 ni en VS 2026 — seulement
  `/std:c++23preview` (ABI non garantie) et `/std:c++latest` (sur-ensemble débordant sur C++26). CMake 4.4 mappe
  `CMAKE_CXX_STANDARD 23` vers `-std:c++latest` chez MSVC. Conséquence : la CI Linux en `-pedantic-errors` fait
  autorité sur la conformité, à mettre en place en M0.2. Trous MSVC à éviter : `[[assume]]` (P1774R8), P2448R2,
  P2582R1, échappements Unicode.
- Relecture : PR #20 relue et fusionnée en **0,25 h** (15 min). ADR-0001 accepté, et avec lui les ADR 0002 à
  0007 (« ça me va »). SPECS passe en v0.3, statut « validé ».
- Cumul M0.1 : **1,25 h passée pour 1,0 h estimée** (ratio 1,25) — l'amendement C++23 n'était pas prévu
  dans l'estimation initiale.
- Prochaine étape : M0.2 — `cxx_std_23` dans les presets CMake et `-pedantic-errors` en CI Linux comme
  garde-fou de conformité, en plus de l'arborescence, de vcpkg et de la CI.

## 2026-09-20 — M0.1 — Dépôt local, modèles GitHub et machine de référence

- Temps Donnovan : à renseigner
- Sessions Claude Code : 1
- Fait : modèles d'issue et de PR déplacés de `tools/github-templates/` vers `.github/` ; dépôt git local initialisé
  (`main`) et premier commit ; SPECS §10 complété à partir de la machine.
- Mesures : machine de référence → CachyOS noyau 7.2.6-1-cachyos, Ryzen 7 7800X3D, RX 9070 XT (GFX1201),
  Mesa RADV 26.2.3-arch3.1, Vulkan 1.4.354, 16 Go (`vulkaninfo --summary`, `uname -r`, `/proc/cpuinfo`,
  `/proc/meminfo`). Outils présents : CMake 4.4.3, Ninja 1.13.2, Clang 22.1.8, GCC 16.2.1, git 2.55.0, gh 2.101.0.
- Écarts et problèmes : deux GPU Vulkan sur la machine (le discret et l'iGPU du 7800X3D) — la sélection de device
  en M1.2 devra préférer le discret ; plusieurs couches Vulkan implicites tierces sont installées et celle de
  Lossless Scaling est cassée (erreur du loader à chaque `vkCreateInstance`). Les deux points sont notés
  dans SPECS §10.
- Décision : licence **MIT** (`LICENSE`), retirée des questions ouvertes de SPECS §11 et inscrite dans les
  conventions (SPECS §8). Compatible avec le code adapté de Donut, qui garde son en-tête MIT.
- Suivi GitHub créé par `./tools/github-bootstrap.sh GameEngine` : dépôt public
  [PhantomDO/GameEngine](https://github.com/PhantomDO/GameEngine), 17 labels, 35 milestones (M0.1 échéance
  27/09/2026 → M8.3 échéance 08/08/2027), board n°1 avec les champs Estimé (h), Passé (h) et Phase, et les
  19 issues des phases 0 et 1, chacune avec son milestone, ses labels, son estimation et sa phase.
- Prochaine étape : M0.2 — arborescence, presets CMake, vcpkg en mode manifeste, CI, puis protection de
  `main` (issue #5). Les issues #1 et #2 (M0.1) attendent Donnovan.

## 2026-09-20 — M0.1 — Machine de référence et vérification Windows

- Temps Donnovan : à renseigner
- Fait : machine de référence renseignée (CachyOS, Ryzen 7 7800X3D, Radeon RX 9070 XT, 16 Go) ; versions d'OS et
  de pilote à compléter avec `vulkaninfo --summary`.
- Décision : pas de machine Windows ; vérification en trois niveaux (CI + WARP, binaire Windows sous Proton sur la
  machine de référence, vraie machine Windows ponctuellement). Une VM Windows écartée : sans passthrough GPU, elle
  n'apporte rien de plus que WARP.
- Dépôt GitHub : pas encore créé ; Claude Code le créera avec `tools/github-bootstrap.sh` après la connexion de
  Donnovan à `gh`. Nom provisoire : GameEngine.

## 2026-09-20 — M0.1 — Révision : NVRHI et flecs

- Temps Donnovan : à renseigner (échange vocal + relecture)
- Sessions Claude Code : 0
- Contexte : Donnovan précise que **la priorité est de faire un jeu avec notre moteur**, et qu'apprendre Vulkan en
  détail ne l'intéresse pas.
- Décisions proposées : NVRHI à la place d'une RHI maison, backends Vulkan + Direct3D 12 (ADR-0002, réécrit) ;
  flecs plutôt qu'EnTT ou un ECS maison, pour sa documentation, son explorer, ses hiérarchies, ses pipelines et sa
  réflexion JSON (ADR-0004, réécrit) ; Slang compilé en SPIR-V et DXIL (ADR-0005, révisé).
- Correction apportée : NVRHI n'est pas la norme de l'industrie (Unreal, Unity et Godot ont leur propre RHI) et
  ne gère ni OpenGL ni Metal (backends : D3D11, D3D12, Vulkan).
- Roadmap v0.2 : 77 h → 68 h Donnovan, 55 → 49 sessions, 35 milestones ; « RHI mince » supprimé ; ajout de M1.4
  (backend D3D12) et M3.5 (choix du jeu) ; le jeu jouable visé début août 2027 à 1,5 h/semaine.
- Ajouts : étude E1 (couches RHI) écrite en avance, `docs/LECTURES.md` (lectures commentées).
- Prochaine étape : inchangée (validation des specs, puis création du dépôt et M0.2).

## 2026-09-20 — M0.1 — Specs initiales

- Temps Donnovan : à renseigner (estimé 1,0 h pour tout M0.1)
- Sessions Claude Code : 0 (rédaction dans une conversation claude.ai)
- Fait : SPECS v0.1, ROADMAP v0.1 (77 h Donnovan, 55 sessions, 34 milestones), ADR 0001 à 0007, CLAUDE.md,
  modèles d'issue et de PR, script `tools/github-bootstrap.sh`.
- Décisions proposées : C++20 sans modules, Vulkan 1.3 avec RHI maison, SDL3, ECS maison à sparse sets, Slang,
  GitHub public, CMake + vcpkg.
- Prochaine étape : Donnovan valide ou amende les specs et les ADR, renseigne la machine de référence, choisit
  le nom ; puis création du dépôt avec le script et démarrage de M0.2 dans Claude Code.
