# `data`

Les données de la démo, lues directement dans le dépôt (`LEVAIN_DATA_DIR`). Tout y est produit par une commande
notée ici, sous la licence du projet (MIT).

| Fichier | Commande |
|---|---|
| `textures/checker.png` | `magick -size 256x256 xc: -fx '(floor(i/32)+floor(j/32))%2 ? 0.25 : 1' -strip PNG24:data/textures/checker.png` |

**C'est une racine d'assets** ([ADR-0019](../docs/adr/0019-identifiants-d-assets.md)) : chaque image ou modèle y
a un `.meta` voisin, qui porte son GUID. Le sandbox le crée au premier lancement ; il se commite avec le fichier,
et le test `assets.metas-committed` échoue s'il a été oublié. Renommer un fichier ici sans son `.meta` ne le perd
pas : le scan le retrouve par le hash de son contenu.
