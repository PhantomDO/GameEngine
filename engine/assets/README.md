# `engine/assets`

## Rôle

Transformer les fichiers du disque en données prêtes pour le moteur. Plus tard : base d'assets par GUID,
cuisson, cache et hot-reload (SPECS §7).

**État en M2.2** : le chargement d'images (`loadImage`, stb_image) et le calcul de leurs mipmaps
(`buildMipChain`), sur le CPU.

## Invariants

1. **Aucun GPU ici.** Le module rend des pixels en mémoire (`Image`) ; c'est `engine/render` qui les envoie au
   GPU. La cuisson des assets pourra donc tourner sur une machine de build sans carte graphique.
2. **stb reste privé** : seul `src/image.cpp` l'inclut, et l'API n'expose que des types standard.
3. **Une image est en RGBA8, couleurs en sRGB**, quel que soit le fichier d'origine (niveaux de gris, RGB sans
   alpha…) : un seul format à gérer en aval.
4. Le module dépend de `core` seul pour l'instant. SPECS §7 le place au-dessus de `scene` : il pourra en
   dépendre quand la base d'assets arrivera, jamais l'inverse.

## Points d'entrée

| Fichier | Contenu |
|---|---|
| [`include/levain/assets/image.hpp`](include/levain/assets/image.hpp) | `Image`, `loadImage`, `mipCountFor`, `buildMipChain` |

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
