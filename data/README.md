# `data`

Les données de la démo, lues directement dans le dépôt (`LEVAIN_DATA_DIR`). Tout y est produit par une commande
notée ici, sous la licence du projet (MIT).

| Fichier | Commande |
|---|---|
| `textures/checker.png` | `magick -size 256x256 xc: -fx '(floor(i/32)+floor(j/32))%2 ? 0.25 : 1' -strip PNG24:data/textures/checker.png` |
