# `engine/assets`

## Rôle

Transformer les fichiers du disque en données prêtes pour le moteur. Plus tard : base d'assets par GUID,
cuisson, cache et hot-reload (SPECS §7).

**État en M4.1** : le chargement d'images (`loadImage`, stb_image), le calcul de leurs mipmaps
(`buildMipChain`), l'écriture de PNG (`savePng`, pour les captures), et l'**import glTF** (`loadGltf`,
fastgltf) : meshes, nœuds et couleur de base des matériaux lus en mémoire, puis instanciés en entités
(`instantiateModel`).

## Invariants

1. **Aucun GPU ici.** Le module rend des pixels en mémoire (`Image`) ; c'est `engine/render` qui les envoie au
   GPU. La cuisson des assets pourra donc tourner sur une machine de build sans carte graphique.
2. **stb reste privé** : seul `src/image.cpp` l'inclut, et l'API n'expose que des types standard.
3. **Une image est en RGBA8, couleurs en sRGB**, quel que soit le fichier d'origine (niveaux de gris, RGB sans
   alpha…) : un seul format à gérer en aval.
4. Le module dépend de `core` et de `scene` : l'import crée des entités. SPECS §7 le place au-dessus de
   `scene`, jamais l'inverse.
5. **fastgltf reste privé** : seul `src/gltf.cpp` l'inclut (`deps.fastgltf-visibility`). L'import rend un
   `Model` fait de nos types, testable sans GPU ni monde flecs.
6. **Un nœud glTF devient une entité**, sous une racine qui déplace tout le modèle, avec son `Transform` et sa
   hiérarchie (`flecs::Parent`, ADR-0015). **Le lien vers le mesh est provisoire** : `MeshInstance` porte un
   indice dans `Model::meshes`, que l'application relie à ses meshes GPU ; la base d'assets de M4.2 le
   remplacera par un handle.
7. **Seules les images de couleur de base sont décodées** : les autres (normal maps, rugosité…) attendront le
   PBR (M5.1). Sponza n'en décode ainsi que 25 sur 69. Une image peut venir d'un fichier, d'octets embarqués
   en base64 ou d'un buffer (`.glb`) : `decodeImage` lit la mémoire, `loadImage` un fichier.

## Points d'entrée

| Fichier | Contenu |
|---|---|
| [`include/levain/assets/image.hpp`](include/levain/assets/image.hpp) | `Image`, `loadImage`, `decodeImage`, `mipCountFor`, `buildMipChain`, `savePng` |
| [`include/levain/assets/gltf.hpp`](include/levain/assets/gltf.hpp) | `Model`, `loadGltf`, `MeshInstance`, `instantiateModel` |

## Les mipmaps

Une texture vue de loin couvre moins de pixels à l'écran qu'elle n'en contient : sans précaution, chaque pixel
de l'écran tombe sur un texel presque au hasard et l'image scintille. Les **mipmaps** sont des copies de la
texture, chacune deux fois plus petite que la précédente, jusqu'à 1 × 1. Le GPU choisit le niveau dont les
texels ont à peu près la taille d'un pixel de l'écran.

**Le piège : la moyenne doit se faire en lumière linéaire.** Les octets d'une image sRGB ne sont pas
proportionnels à la lumière. Leur moyenne directe assombrit chaque niveau : un damier noir et blanc devient un
gris à 128 au lieu de 188, et une texture paraît plus sombre de loin que de près. `downsampleInLinearSpace`
convertit en linéaire, fait la moyenne, puis reconvertit (test « buildMipChain fait la moyenne en lumière
linéaire »).

**Pourquoi sur le CPU** : c'est le plus simple (aucun pipeline de calcul), et la cuisson des assets fera ce
travail hors ligne un jour. Le filtre est écrit à la main plutôt que pris dans stb_image_resize2, qui déclenche
UBSan (`.agents/skills/build/GOTCHA.md`). La génération sur le GPU, par un compute shader comme dans Donut, se
justifiera si des textures sont créées à l'exécution.

## Équivalents ailleurs

| Moteur | Où | Ce qu'on y trouve |
|---|---|---|
| **Unreal** | Texture Import, `TextureCompressor` | Les mips sont calculées à l'import et à la cuisson, jamais à l'exécution (**documenté** : sources publiques). |
| **Unity** | Texture Importer, option *Generate Mip Maps* | Calcul à l'import ; l'option *sRGB (Color Texture)* dit si la moyenne se fait en linéaire (**documenté** : manuel). |
| **Godot** | `Image::generate_mipmaps` | Calcul à l'import, avec une option pour les images sRGB (**documenté** : dépôt public). |
| **Unreal** | Interchange, importeur glTF | Un glTF devient des Static Meshes, et ses nœuds des Actors dans le niveau (**documenté** : documentation d'Epic). |
| **Unity** | package glTFast | Import à l'exécution ou dans l'éditeur, les nœuds deviennent des GameObjects (**documenté** : manuel du package). |
| **Godot** | `GLTFDocument` | Le format 3D recommandé ; un glTF s'importe comme une scène de nœuds (**documenté** : documentation officielle). C'est le plus proche d'ici : une entité par nœud. |
