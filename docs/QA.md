# Questions et réponses

Les questions de Donnovan dont la réponse mérite d'être retrouvée. La plus récente en haut. Chaque réponse
renvoie au code, à un ADR ou à une source.

## Format

```
### Question (AAAA-MM-JJ, Mx.y)
Réponse courte, puis détails. Références : fichier:ligne, ADR, source externe.
```

---

### Quiz sur l'étude E1 : les trois réponses à retenir (2026-09-21, M1.3)

Issue #17 faite par sondages : 5 bonnes réponses sur 8. Les trois erreurs portent sur la liaison des ressources,
le sujet de M2.1 — les voici corrigées.

**Pourquoi des décalages de binding (`-fvk-b-shift 256`…) pour Vulkan ?** HLSL sépare les registres par type :
`t0` (texture) et `b0` (constantes) ne se gênent pas. Vulkan n'a qu'une numérotation par descriptor set : le
binding 0 ne peut désigner qu'une ressource. NVRHI découpe donc l'espace en plages — textures à 0, samplers à
128, constantes à 256, UAV à 384 (`nvrhi::VulkanBindingOffsets`) —, et `slangc` doit appliquer les mêmes
(`shaders/CMakeLists.txt`). Sinon NVRHI lie les constantes au binding 256 et le shader lit le 0. Le DXIL, lui,
garde ses registres séparés et n'a besoin d'aucun décalage.

**Binding set ou descriptor table ?** Les deux existent sur les deux backends. Le *binding set* est immuable :
ses descripteurs sont écrits à la création, et NVRHI garde vivantes ses ressources et place leurs barrières. La
*descriptor table* (bindless) est un grand tableau modifiable que le shader indexe, par exemple par numéro de
matériau : NVRHI n'y suit **rien**. C'est la voie des gros moteurs et du ray tracing, et le choix de l'ADR de M2.1
(#40). Source : NVRHI Programming Guide (E1, §4).

**Qu'est-ce qu'un volatile constant buffer ?** Un buffer de constantes bien suivi par NVRHI, dont le **contenu**
est éphémère : il n'existe qu'entre le premier `writeBuffer` et la fermeture de la command list. NVRHI puise dans
un anneau de buffers d'upload à notre place ; sans lui, il faudrait un buffer par frame en vol, géré à la main.
C'est ce qui portera les matrices de la caméra en M2.1.

### Si on remet Windows, le moteur est-il prêt pour Direct3D 12, ou y aura-t-il beaucoup de travail ? (2026-09-21, M1.2)

Réponse courte : **c'est pensé pour, et le gros du travail n'est pas graphique.**

**Déjà interchangeable.**

- Tout ce qui s'écrit au-dessus de `nvrhi::IDevice` — command lists, pipelines, textures, passes de rendu —
  tourne sous Direct3D 12 sans changement : c'est la raison d'être de NVRHI ([ADR-0002](adr/0002-nvrhi.md)).
  `engine/render`, le plus gros du moteur, n'en saura rien.
- Hors de `engine/gpu`, un seul endroit connaît Vulkan : le drapeau `SDL_WINDOW_VULKAN` de
  `engine/platform/src/window.cpp` (vérifié par `grep` le 2026-09-21). SDL3 gère Windows.
- **Vulkan tourne aussi sous Windows.** Remettre Windows ne demande pas Direct3D 12 : le backend actuel y
  fonctionnerait tel quel. D3D12 serait un second backend, optionnel.

**À écrire pour Direct3D 12.**

1. `device_d3d12.cpp` à côté de `device_vk.cpp` : factory DXGI, adaptateur, device, queue, couche de debug, puis
   `nvrhi::d3d12::createDevice`. Donut y consacre **606 lignes, swapchain comprise, contre 1 413 pour Vulkan**
   (`DeviceManager_DX12.cpp`, `DeviceManager_VK.cpp`, mesuré avec `wc -l`) : D3D12 n'a pas la cérémonie
   d'instance, d'extensions et de fonctionnalités de Vulkan.
2. La swapchain DXGI, l'équivalent de `swapchain_vk.cpp`.
3. Les shaders compilés aussi en DXIL : prévu, Slang produit SPIR-V et DXIL ([ADR-0005](adr/0005-shaders-slang.md)),
   et nos shaders suivent déjà la convention de slots HLSL de NVRHI.
4. Le choix du backend au lancement (`--api vulkan|d3d12`).
5. **Le plus lourd, et rien de graphique** : presets et CI Windows, choix du compilateur (clang-cl d'abord,
   [ADR-0011](adr/0011-retour-au-cpp.md)), test de fumée sous WARP.

**Le seul couplage de notre API** : `engine/gpu/include/levain/gpu/device.hpp` nomme un type opaque
`VulkanContext` et un membre `vulkan`. Un second backend demandera un nom neutre, avec une définition par fichier
de backend. Un renommage de quelques lignes, pas fait tant que Windows est hors périmètre.

**Chez les autres** (documenté, sources publiques) : Unreal a une interface `FDynamicRHI`, avec les modules
`D3D12RHI` et `VulkanRHI` choisis au lancement (`-d3d12`, `-vulkan`) ; Godot a des `RenderingDeviceDriver`
Vulkan, D3D12 (depuis la 4.3) et Metal. Chez nous, NVRHI joue le rôle de leur RHI : il ne reste, par backend, que
la création du device et de la swapchain.

### Clang existe aussi sous Windows — pourquoi prendre MSVC ? (2026-09-20, M0.5)

Question posée à la clôture de M0.5. Réponse : **clang-cl ne règlerait aucun des deux bugs de M0.2**, ce qui est
contre-intuitif, mais il réglerait un problème plus profond.

**Ce que clang-cl ne règle pas.** La [doc de compatibilité MSVC de Clang](https://clang.llvm.org/docs/MSVCCompatibility.html)
est explicite sur deux points :

1. **clang-cl reproduit volontairement le bug `__cplusplus`.** MSVC prétend être en C++98 ; clang-cl imite ce
   comportement par compatibilité, donc `/Zc:__cplusplus` reste nécessaire.
2. **clang-cl consomme la STL de Microsoft.** C'est son principe même : remplacer `cl.exe` en utilisant les
   en-têtes et bibliothèques MSVC. La divergence `<ostream>` venait de la STL, pas du compilateur — elle serait
   identique.

Nos deux bugs de M0.2 étaient des bugs de **bibliothèque et de driver**, pas de compilateur.

**Ce que clang-cl règle, et c'est le point important.** Avec MSVC on est coincés sur `/std:c++latest`, qui
déborde sur le brouillon C++26 : mesuré à `__cplusplus 202400` sous Windows contre `202302` sous Linux
(journal, M0.2). Avec clang-cl, `-std=c++23` donne **exactement C++23 sur les deux plateformes**. Le garde-fou
`-pedantic-errors` de l'ADR-0001 redeviendrait une précaution au lieu d'une nécessité. S'ajoutent les mêmes
diagnostics, warnings, clang-tidy et clang-format partout — fini l'épinglage de version d'un seul côté.

**La réserve** : la même doc précise que le support de l'ABI C++ de MSVC par Clang est « a work in progress ».
Non négligeable pour un moteur qui lie des dépendances compilées par vcpkg.

**La troisième voie** : clang + MinGW-w64 + libstdc++ donnerait *la même bibliothèque standard que sous Linux*,
donc plus de divergence de STL du tout. Mais ABI différente, support du SDK Windows et de Direct3D 12 plus
rugueux, et triplets mingw de vcpkg de qualité communautaire. Plus risqué.

**Décision** : sans objet tant que Windows est hors périmètre (ADR-0011). **Quand Windows redeviendra une cible,
clang-cl est la première option à évaluer** — et non MSVC par défaut, comme l'ADR-0001 l'avait posé sans le
justifier.

### Pourquoi « Levain » ? (2026-09-20, M0.1)

Parce que c'est le rythme du projet. Un levain se nourrit un peu chaque semaine, reste vivant entre deux
fournées, et sert de **base à partir de laquelle on cuit autre chose** — ce qu'est un moteur par rapport à un
jeu. Il se partage aussi, ce qui colle au « cuisiner à plusieurs » de Donnovan.

Le champ culinaire était déjà présent dans le projet avant le nom : M4.3 de la roadmap s'appelle
« Cuisson des assets » (*asset baking*).

Écartés et pourquoi : **Blitter** (la puce Amiga/Atari ST — Donnovan a commencé sur Game Boy Color et
PlayStation, la référence ne lui parlait pas) ; **Encore**, **Ludus**, **Noria**, **Marmite**, **Brigade**
(collisions GitHub sérieuses, dont `ludusavi`, un outil de sauvegardes de jeux — même domaine) ; **Braise**
(sémantiquement décroissante, elle pointe vers le passé) ; **Mijote** (verbe conjugué, mauvais namespace) ;
**Madeleine** et **Cartouche** (muets sur la technique).

Le namespace `levain` est une exception assumée à la règle « identifiants en anglais » (SPECS §8) : c'est un nom
propre, il ne se traduit pas, comme Godot.

### Le C++23 est-il stable en 2026 ? Peut-on y passer ? (2026-09-20, M0.1)

Oui côté Linux, « pas officiellement » côté Windows — et c'est gérable.

**Linux** : mesuré sur la machine de référence, Clang 22.1.8 et GCC 16.2.1 avec libstdc++ 16 donnent
`__cplusplus == 202302` et **toutes** les fonctionnalités C++23 sondées (dont `std::expected`, `std::print`,
`std::stacktrace`, `std::mdspan`, deducing this). Rien ne manque.

**Windows** : `/std:c++23` **n'existe pas**, ni en Visual Studio 2022 ni en Visual Studio 2026. Microsoft ne
propose que `/std:c++23preview` (« peut changer, peut ne pas être compatible en ABI d'une version à l'autre ») et
`/std:c++latest` (sur-ensemble qui déborde sur le brouillon C++26). CMake 4.4 mappe `CMAKE_CXX_STANDARD 23` vers
`-std:c++latest` chez MSVC (`/usr/share/cmake/Modules/Compiler/MSVC-CXX.cmake:46`). Les deux moitiés de la
matrice ne compilent donc pas le même langage.

**La parade** : c'est Linux qui fait autorité. La CI Linux compile en `-std=c++23 -pedantic-errors`, ce qui
refuse toute fonctionnalité post-C++23 (vérifié sur l'indexation de paquets `P2662`, refusée par Clang et par
GCC). Le code qui passe sous Linux est du C++23 ; MSVC, plus permissif, ne peut pas introduire de dérive
silencieuse.

Le déclencheur du changement est `std::expected` : la politique de gestion d'erreurs se décide en M0.3
(ADR-0008), et la trancher sans `std::expected` sous la main aurait appauvri le choix.

**Confirmé par la CI le 20/09/2026**, et le chiffre est parlant. Avec `/Zc:__cplusplus`, le même sandbox
compilé depuis les mêmes sources annonce :

| Plateforme | Compilateur | `__cplusplus` |
|---|---|---:|
| Linux | Clang 18.1.3, `-std=c++23` | **202302** — exactement C++23 |
| Windows | MSVC 19.51.36256, `/std:c++latest` | **202400** — au-delà de C++23 |

C'est la démonstration directe du raisonnement de l'ADR-0001 : les deux moitiés de la matrice ne compilent pas
le même langage, et Windows est plus permissif. `202400` n'est la valeur d'aucune norme publiée — c'est un mode
brouillon post-C++23. D'où le `-pedantic-errors` côté Linux : sans lui, rien n'empêcherait d'écrire du C++26 qui
passerait sous Windows.

Piège associé : **MSVC épingle `__cplusplus` à `199711L`** (la valeur de C++98) tant qu'on ne passe pas
`/Zc:__cplusplus`, quelle que soit la vraie version. Le premier run de CI l'a montré. Tout `#if __cplusplus >=
202302L` prendrait donc la mauvaise branche sous Windows, sans le moindre avertissement. Le flag est posé dans
le `CMakeLists.txt` racine, avec un commentaire pour qu'on ne le retire pas.

Détails, mesures et liste des trous MSVC restants : [ADR-0001](adr/0001-langage-cpp23.md).
Sources : [`/std` (msvc-180)](https://learn.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version?view=msvc-180),
[conformance C/C++ Microsoft](https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance).

### NVRHI est-il la norme de l'industrie ? Gère-t-il OpenGL et Metal ? (2026-09-20, M0.1)

Non aux deux. NVRHI est la couche de NVIDIA, utilisée par ses SDK RTX et ses exemples (Donut), et par quelques
projets tiers comme RBDOOM-3-BFG. Les grands moteurs ont chacun leur propre RHI (Unreal, Godot, Unity). Ses
backends sont Direct3D 11, Direct3D 12 et Vulkan : pas d'OpenGL ni de Metal. C'est sans conséquence pour nos
plateformes (Windows, Linux). On l'a choisi parce qu'il nous évite d'écrire une RHI, ce qui sert la priorité
« faire un jeu ». Détails : [ADR-0002](adr/0002-nvrhi.md), [étude E1](etudes/E1-rhi.md).

### NVRHI fonctionne-t-il sur une carte AMD ? (2026-09-20, M0.1)

Oui. NVRHI est une couche au-dessus de Vulkan et de Direct3D 12, deux API standard ; il tourne sur n'importe quel
GPU qui les prend en charge, dont la Radeon RX 9070 XT de la machine de référence. Seules les extensions propres à
NVIDIA (NVAPI, désactivée par défaut) sont réservées à ses cartes : on ne les utilise pas.

### Faut-il une VM Windows, ou Proton suffit-il ? (2026-09-20, M0.1)

Proton est plus utile : il lance le binaire Windows sur le vrai GPU, en traduisant Direct3D 12 en Vulkan
(vkd3d-proton). Une VM sans passthrough GPU n'a que du rendu logiciel, comme WARP en CI. Limite de Proton : ce
n'est pas un pilote D3D12 natif. Détails : SPECS §10.

### flecs ou EnTT ? (2026-09-20, M0.1)

flecs : documentation plus riche (quickstart, manuels par sujet, articles de l'auteur), explorer web, hiérarchies
et pipelines prêts à l'emploi, réflexion et JSON intégrés, utiles pour l'éditeur. EnTT est excellent et plus
minimal : il laisse davantage à écrire. Les deux sont bien maintenus. Détails : [ADR-0004](adr/0004-ecs-flecs.md).

### Faut-il utiliser un autre langage que le C++ pour un moteur en 2026 ? (2026-09-20, M0.1)

Non, pour ce projet. Rust, Zig et Odin sont de vraies alternatives, mais tous les moteurs étudiés sont en C++,
toutes les bibliothèques retenues aussi, et l'objectif est de comprendre les moteurs, pas d'apprendre un langage.
Détails : [ADR-0001](adr/0001-langage-cpp23.md).

### GitHub, GitLab ou Azure DevOps ? (2026-09-20, M0.1)

GitHub en dépôt public : CI Windows + Linux gratuite et illimitée, board avec champs personnalisés, CLI `gh`
pilotable par Claude Code. Le seul vrai atout de GitLab est le suivi du temps natif, compensé ici par des champs
du board et le journal. Détails : [ADR-0006](adr/0006-hebergement-github.md).
