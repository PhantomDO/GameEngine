# E2 — Ressources GPU et shaders

> Écrite le 22/09/2026, à la clôture de la phase 2. Lecture : 15 à 20 minutes.
> Convention : ce qui est **documenté** renvoie à une source ; ce qui est **déduit** est signalé.

## La question

La phase 2 a rencontré deux problèmes que tous les moteurs résolvent, chacun à sa façon :

1. **La liaison** : comment un shader trouve-t-il ses ressources, textures, buffers et samplers ?
2. **La compilation** : comment un shader passe-t-il du texte au code que le GPU exécute, sans que le joueur voie
   de saccade, et sans que le développeur attende ?

## 1. Ce que fait notre moteur (et pourquoi)

- **Liaison par binding sets**, rangés par fréquence de changement : `space0` pour la frame, `space1` pour la
  passe, `space2` pour le matériau ([ADR-0013](../adr/0013-binding-sets.md)). NVRHI garde les ressources vivantes
  et place leurs barrières.
- **Compilation au build** : `slangc` produit le SPIR-V et le DXIL de chaque point d'entrée
  ([ADR-0005](../adr/0005-shaders-slang.md)). Le moteur crée ses pipelines à la création des passes, jamais
  pendant une frame.
- **Hot-reload** en relançant ce même build, en ~450 ms ; seul le pipeline est recréé, en 0,2 à 2 ms
  ([ADR-0014](../adr/0014-hot-reload-des-shaders.md)).
- **Pas de variantes** de shaders, et aucun cache de pipelines à nous : seul celui du pilote sert.

## 2. Binding sets et bindless dans NVRHI (documenté [1])

- Un **binding layout** déclare les slots qu'utilisent les shaders et le type de chaque ressource ; un **binding
  set** y place les ressources réelles. Les deux sont immuables.
- Le binding set garde une **référence forte** sur ses ressources : NVRHI sait qui les utilise, et peut suivre
  leur durée de vie et leurs états.
- Le **bindless** passe par des *descriptor tables*, de taille variable, que le shader indexe librement. D'après
  le guide, ces tables ne gardent pas de référence forte sur leurs ressources : aucun suivi de durée de vie, et
  les barrières sont à la charge de l'application.

Le bindless est indispensable au ray tracing et au rendu piloté par le GPU ; ni l'un ni l'autre n'est dans la
v1. D'où le choix de l'ADR-0013, et la règle : **passer au bindless demandera un nouvel ADR**.

## 3. Du texte au GPU : trois étapes, trois coûts

| Étape | Qui la fait, et où | Coût | Chez nous |
|---|---|---|---|
| **Compilation** du shader : source → bytecode (SPIR-V, DXIL) | un compilateur, hors du jeu | ~150 ms par point d'entrée (`slangc`, mesuré en M2.3) | au build |
| **Variantes** : une compilation par combinaison d'options | l'outil de build ou l'éditeur | se multiplie : 4 options à 2 valeurs = 16 variantes | aucune |
| **Pipeline** : bytecode + états (profondeur, faces…) → code machine du GPU | le pilote, **sur la machine du joueur** | de quelques ms à quelques centaines de ms [4] | à la création des passes |

La troisième étape est la source des fameuses **saccades à la première apparition** d'un effet : si le pipeline
n'existe pas encore quand un objet entre dans le champ, le jeu attend que le pilote le compile [4][9].

## 4. Unreal Engine (documenté)

- **Compilation hors du moteur** : des processus `ShaderCompileWorker` compilent les shaders. Depuis la 5.4, le
  prétraitement se fait dans l'éditeur ou le processus de cuisson, plus dans les workers [2].
- **Cache par empreinte** : les shaders compilés vont dans le *Derived Data Cache*, sous une empreinte de toutes les
  entrées de la compilation, sources comprises. Une source modifiée est reprise au prochain lancement, ou par la
  commande `recompileshaders changed`, « la façon la plus rapide d'itérer » sur un shader global [3].
- **Permutations** : chaque option statique multiplie le nombre de shaders à compiler, 2ⁿ pour n interrupteurs [5].
- **Pipelines** : le *PSO Precaching* compile en tâche de fond, dès le chargement d'un asset, tous les pipelines
  qu'il pourrait utiliser. Les composants de rendu lancent cette précompilation juste après leur chargement [4].

## 5. Unity (documenté)

- **Compilation hors du moteur** : des processus `UnityShaderCompiler`, en général un par cœur, compilent en
  parallèle au build [6].
- **Variantes par mots-clés** : un mot-clé `multi_compile` produit toujours ses variantes ; un mot-clé
  `shader_feature` ne les produit que si un matériau du projet s'en sert. Le *stripping* retire les variantes
  inutiles, ce qui réduit le temps de build, la taille, le temps de chargement et la mémoire. S'il manque une
  variante à l'exécution, Unity en choisit une proche [7].
- **Dans l'éditeur** : une variante pas encore compilée l'est en tâche de fond, et l'objet s'affiche en cyan
  uni en attendant [8].

## 6. Godot 4 (documenté)

- **Liaison** : `RenderingDevice` n'a que des *uniform sets*, l'équivalent des binding sets [11].
- **Pipelines** : Godot appelle *pipeline compilation* l'étape où le pilote convertit le SPIR-V en code pour le
  GPU, sur la machine du joueur. Avant la 4.4, les pipelines se compilaient quand un objet entrait dans le champ,
  d'où des saccades au premier passage [9].
- **Ubershaders (4.4)** : une version de chaque shader qui lit ses options (des *specialization constants*)
  pendant le rendu. Un seul pipeline est précompilé au chargement, et les versions optimisées se compilent en
  tâche de fond pendant le jeu [9][10].

## 7. Ce qu'on en retient

- **Notre liaison est celle de Godot et de Donut** : des binding sets par fréquence. Le bindless attendra un
  besoin mesuré.
- **Unreal et Unity compilent leurs shaders hors du moteur**, dans des processus séparés : c'est aussi notre
  choix pour le hot-reload (ADR-0014). Godot compile dans son propre processus.
- **Les deux problèmes qui nous attendent** (déduit de ce qui précède) :
  - les **variantes**, dès que les matériaux auront des options (carte de normales ou non, ombres ou non). Les
    trois moteurs montrent le prix de 2ⁿ ; les *specialization constants*, comme les ubershaders de Godot,
    ou quelques permutations choisies, sont les pistes à comparer dans un ADR ;
  - les **saccades de pipelines**, quand le nombre de matériaux grandira. Notre règle actuelle, des pipelines
    créés à la création des passes, est la forme la plus simple de la précompilation d'Unreal et de Godot. Elle
    tiendra tant que les pipelines seront connus au chargement.
- **Ce qui manque à notre hot-reload** (déduit) : un cache par empreinte, comme le DDC d'Unreal. Ninja ne
  recompile déjà que les fichiers modifiés, et c'est suffisant pour quatre points d'entrée.

## 8. Ce que la phase 2 a mesuré

| Mesure | Valeur | Milestone |
|---|---|---|
| 10 000 cubes instanciés, un seul draw | 0,043 ms de GPU, 120 images/s (l'écran) | M2.1 |
| Mips moyennées en lumière linéaire | 192 au lieu de 160 sur le dernier niveau | M2.2 |
| Filtrage anisotrope ×16 | contraste à l'horizon ×1,8 ; +0,025 ms de GPU | M2.2 |
| Hot-reload d'un shader | ~450 ms, dont 350 de build | M2.3 |

## Sources

1. NVIDIA, *NVRHI Programming Guide*, sections « Binding Layouts and Sets » et « Bindless Resources » —
   https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/ProgrammingGuide.md
2. Epic Games, *Debugging the Shader Compile Process in Unreal Engine* —
   https://dev.epicgames.com/documentation/en-us/unreal-engine/debugging-the-shader-compile-process-in-unreal-engine
3. Epic Games, *Shader Development in Unreal Engine* —
   https://dev.epicgames.com/documentation/unreal-engine/shader-development-in-unreal-engine
4. Epic Games, *PSO Precaching for Unreal Engine* —
   https://dev.epicgames.com/documentation/en-us/unreal-engine/pso-precaching-for-unreal-engine
5. Epic Developer Community, *Knowledge Base: Understanding Shader Permutations* —
   https://forums.unrealengine.com/t/knowledge-base-understanding-shader-permutations/264928
6. Unity, *Shader compilation* — https://docs.unity3d.com/6000.2/Documentation/Manual/shader-compilation.html
7. Unity, *Strip shader variants* — https://docs.unity3d.com/6000.1/Documentation/Manual/shader-variant-stripping.html
8. Unity, *Asynchronous shader compilation in the Editor* —
   https://docs.unity3d.com/Manual/AsynchronousShaderCompilation.html
9. Godot, *Reducing stutter from shader (pipeline) compilations* —
   https://docs.godotengine.org/en/stable/tutorials/performance/pipeline_compilations.html
10. Godot, PR #90400 *Ubershaders and pipeline pre-compilation* — https://github.com/godotengine/godot/pull/90400
11. Godot, `servers/rendering/rendering_device.h` —
    https://github.com/godotengine/godot/blob/master/servers/rendering/rendering_device.h
