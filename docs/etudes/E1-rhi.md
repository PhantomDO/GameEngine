# E1 — Les couches RHI

> Écrite le 20/09/2026, avant la phase 1, pour préparer la lecture du code NVRHI. Lecture : 20 à 25 minutes.
> Convention : ce qui est **documenté** renvoie à une source ; ce qui est **déduit** est signalé.

## La question

Pourquoi aucun grand moteur ne parle directement à Vulkan ou à Direct3D 12 ? Que fait exactement la couche
intermédiaire, la RHI (*Rendering Hardware Interface*) ? Et où se place NVRHI dans ce paysage ?

## 1. Ce que les API modernes laissent à la charge du moteur

Vulkan, Direct3D 12 et Metal sont des API **explicites** : le pilote fait très peu de choses implicitement, en
échange d'un surcoût CPU plus faible et d'un contrôle plus fin [1][2]. Le moteur hérite donc de problèmes que le
pilote réglait autrefois (OpenGL, Direct3D 11) :

| Problème | Ce que ça veut dire | Si on se trompe |
|---|---|---|
| Synchronisation | Dire au GPU quand une texture passe de « cible de rendu » à « lue par un shader » (barrières) | Artefacts, lecture d'une image à moitié écrite |
| Durée de vie | Ne pas détruire une ressource que le GPU utilise encore (le CPU a souvent une ou deux frames d'avance) | Plantages ou corruptions aléatoires |
| Mémoire | Allouer de gros blocs et les découper soi-même | Lenteur, dépassement des limites du pilote |
| Liaison des ressources | Dire aux shaders où trouver textures et buffers (descripteurs) | Mauvaise ressource lue, crash du pilote |
| Envoi de données | Passer par des buffers intermédiaires pour copier vers la mémoire GPU | Blocages ou données corrompues |
| Pipelines précompilés | Chaque combinaison shaders + états doit être compilée à l'avance | Saccades la première fois qu'un effet apparaît |

Et chaque API a son propre vocabulaire pour ces notions [2]. Une RHI existe pour écrire le renderer **une seule
fois**, et pour régler ces problèmes à un seul endroit.

## 2. Trois niveaux d'abstraction

```
Gameplay
Renderer : passes (ombres, opaque, transparents, post-process)
───────────────────────────────────────────────────────────────
Niveau 3  Graphe de rendu      FrameGraph (Frostbite), RDG (Unreal), Render Graph (Unity)   ← candidat v2 chez nous
Niveau 2  RHI                  RHI (Unreal), RenderingDevice (Godot), NVRHI                  ← NVRHI chez nous
Niveau 1  API native           Vulkan · Direct3D 12 · Metal
───────────────────────────────────────────────────────────────
Pilote et GPU
```

- **Niveau 2, la RHI** : une interface commune (ressources, command lists, pipelines, liaison) traduite vers
  chaque API. Selon les RHI, elle place aussi les barrières et gère la durée de vie des ressources.
- **Niveau 3, le graphe de rendu** : le renderer déclare ses passes et les ressources que chacune lit ou écrit ;
  le moteur en déduit l'ordre d'exécution, les barrières, et peut réutiliser la mémoire des ressources temporaires.
  L'idée a été popularisée par Frostbite (GDC 2017) [9].

## 3. Comment font les grands moteurs

### Unreal Engine (documenté)

- La RHI est « l'interface multiplateforme vers les différentes API graphiques » prises en charge [3].
- Trois threads coopèrent : le **game thread** produit la logique ; le **render thread** sert de *frontend* et met
  en file des commandes graphiques indépendantes de la plateforme dans une command list ; le **RHI thread**
  les traduit vers l'API réelle [3].
- Chaque commande est une structure dérivée de `FRHICommand` qui implémente `Execute` ; à la traduction, elle
  appelle l'interface `IRHICommandContext` du backend [3].
- Sur Direct3D 12, Vulkan et consoles, la traduction peut se faire en parallèle [3].
- Au-dessus : le Render Dependency Graph (RDG), un graphe de rendu (niveau 3).

### Godot 4 (documenté, et code source ouvert)

- `RenderingDevice` est l'abstraction maison, d'un niveau comparable à WebGPU [4].
- Trois drivers : **Vulkan** (le principal, socle 1.0), **Direct3D 12** (shaders SPIR-V convertis en DXIL via NIR,
  de Mesa) et **Metal** (GLSL converti en MSL via SPIRV-Cross) [4].
- Les renderers Forward+ et Mobile passent par `RenderingDevice` ; le renderer **Compatibility** (OpenGL) le
  contourne, car OpenGL précède le modèle des API explicites [4].
- Depuis Godot 4.3, un **graphe de commandes acyclique** interne à `RenderingDevice` réordonne les commandes et
  place les barrières automatiquement : un intermédiaire entre les niveaux 2 et 3 [5].
- Pour lire une RHI de production complète, `servers/rendering/rendering_device.h` dans le dépôt de Godot est le
  meilleur point d'entrée.

### Unity (en partie documenté)

- Le cœur du moteur est en C++, avec une couche graphique interne **non documentée publiquement** (déduit : elle
  joue le rôle d'une RHI).
- Le Scriptable Render Pipeline (SRP) est « une fine couche d'API » en C# pour programmer les commandes de rendu ;
  `ScriptableRenderContext` fait l'interface entre ce code C# et le code graphique bas niveau de Unity [6].
- URP et HDRP, écrits sur le SRP, s'appuient sur le système Render Graph (niveau 3) [7].

## 4. NVRHI en détail

NVIDIA décrit son modèle comme « un mélange de D3D11 et de D3D12, avec une touche de Vulkan » [8]. Concrètement :

| Notion | Dans NVRHI | Ce que ça nous évite |
|---|---|---|
| Device | `nvrhi::IDevice`, créé **en enveloppant** un device natif que l'application a créé (`nvrhi::vulkan::createDevice`, `nvrhi::d3d12::createDevice`) | Rien : créer le device et la swapchain reste notre travail (le `DeviceManager`, comme dans Donut) |
| Commandes | `ICommandList` : `open()`, enregistrement, `close()`, puis exécution par le device ; plusieurs listes peuvent être enregistrées en parallèle | La gestion des pools de command buffers et leur recyclage |
| Liaison | *Binding layout* (déclare les slots `t`, `u`, `b`, `s` attendus par le shader) + *binding set* (les ressources réelles, immuable, descripteurs pré-remplis à la création) | L'allocation et l'écriture des descripteurs |
| Bindless | *Descriptor tables* : tableaux modifiables de ressources, **sans** suivi de durée de vie ni barrières automatiques | — (c'est à nous d'être prudents) |
| Constantes par frame | *Volatile constant buffers* : n'existent qu'entre le premier `writeBuffer` et la fermeture de la command list | Les buffers circulaires d'upload |
| Pipelines | Graphics, compute, meshlet, ray tracing ; immuables ; ne dépendent que du format des cibles (`FramebufferInfo`) | — |
| Synchronisation | Suivi automatique des états et placement des barrières, désactivable ; trois façons de déclarer les états (explicite, `keepInitialState`, états permanents pour les ressources statiques) | L'essentiel du travail de barrières |
| Durée de vie | Comptage de références (`RefCountPtr`, comme `ComPtr`) ; destruction différée tant que le GPU utilise la ressource ; **`runGarbageCollection()` à appeler une fois par frame** | Les files de destruction différée |
| Upload | `writeBuffer`, `writeTexture` via un gestionnaire interne à la command list (qui ne rétrécit jamais) | Les buffers de staging |
| Validation | `nvrhi::validation::createDevice` enveloppe le device et détecte des erreurs que les couches Vulkan ou D3D12 ne voient pas | — |
| Shaders | NVRHI **ne compile pas** les shaders ; ShaderMake (FXC, DXC ou Slang → DXBC, DXIL ou SPIR-V) s'en charge [10] | — |

Sources : [8] pour tout le tableau, sauf la ligne Shaders.

Ce que NVRHI ne fait pas : créer le device et la swapchain, fournir un graphe de rendu, gérer Metal ou OpenGL
(backends : D3D11, D3D12, Vulkan 1.3) [11].

## 5. Les autres RHI open-source, pour situer

- **SDL_GPU** : dans SDL3, backends Vulkan, D3D12 et Metal ; volontairement réduite à l'essentiel.
- **WebGPU natif** (Dawn chez Google, wgpu en Rust) : l'API du web, utilisable hors navigateur.
- **Diligent Engine**, **bgfx** : RHI multi-backend matures, chacune avec son propre modèle.
- **Donut** : pas une RHI mais un framework de rendu au-dessus de NVRHI ; NVIDIA précise que ce n'est pas un
  moteur de jeu [12].

Pour une vision critique de toute cette complexité (explosion du nombre de PSO, caches de pipelines locaux qui
atteignent 100 Go), voir *No Graphics API* de Sebastian Aaltonen [13].

## 6. Ce qu'on en retient pour notre moteur

1. **NVRHI occupe la place de la RHI d'Unreal ou du `RenderingDevice` de Godot** (niveau 2). Notre code au-dessus :
   le `DeviceManager` par backend, les passes du renderer, et plus tard un graphe de rendu (v2).
2. **Trois choses à maîtriser pour lire notre code de rendu** : binding layouts et binding sets ; les trois modes
   de suivi d'état ; l'appel à `runGarbageCollection()` à chaque frame.
3. **Piège** : NVRHI raisonne en slots HLSL et applique des décalages sous Vulkan. Nos shaders suivent cette
   convention (ADR-0005).
4. **Pas de RHI thread en v1** : on enregistre et on soumet depuis le thread principal. NVRHI permet des command
   lists en parallèle (voir l'exemple Threaded Rendering de Donut-Samples) : ce sera un sujet de la v2.
5. Si la curiosité l'emporte, le code de `RenderingDevice` dans Godot est la RHI de production la plus facile à
   lire.

## Sources

1. Arseny Kapoulkine, *Writing an efficient Vulkan renderer* (2020) —
   https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/ ; traduction française de Dorian Fevrier :
   https://www.fevrierdorian.com/carnet/pages/ecrire-un-moteur-de-rendu-vulkan-performant.html
2. Alain Galvan, *A Comparison of Modern Graphics APIs* — https://alain.xyz/blog/comparison-of-modern-graphics-apis
3. Epic Games, *Parallel Rendering Overview* (Unreal Engine 5.8) —
   https://dev.epicgames.com/documentation/unreal-engine/parallel-rendering-overview-for-unreal-engine
4. Godot, *Internal rendering architecture* —
   https://docs.godotengine.org/en/4.6/engine_details/architecture/internal_rendering_architecture.html
5. Godot, *GPU synchronization in Godot 4.3 is getting a major upgrade* —
   https://godotengine.org/article/rendering-acyclic-graph/
6. Unity, *Scriptable Render Pipeline fundamentals* —
   https://docs.unity3d.com/6000.5/Documentation/Manual/scriptable-render-pipeline-introduction.html
7. Unity, *Render graph* (SRP Core) —
   https://docs.unity3d.com/Packages/com.unity.render-pipelines.core@17.1/manual/render-graph-writing-a-render-pipeline.html
8. NVIDIA, *NVRHI Programming Guide* — https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/ProgrammingGuide.md
9. Yuriy O'Donnell, *FrameGraph: Extensible Rendering Architecture in Frostbite* (GDC 2017) —
   https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in
10. NVIDIA, *ShaderMake* — https://github.com/NVIDIA-RTX/ShaderMake
11. NVIDIA, *NVRHI* (README) — https://github.com/NVIDIA-RTX/NVRHI
12. NVIDIA, *Donut* — https://github.com/NVIDIA-RTX/Donut
13. Sebastian Aaltonen, *No Graphics API* — https://www.sebastianaaltonen.com/blog/no-graphics-api
