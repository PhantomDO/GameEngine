---
name: questions
description: Répondre aux questions de Donnovan sur Levain ou sur les moteurs de jeu — citer le code, expliquer le pourquoi, comparer avec Unreal, Unity et Godot en distinguant documenté et supposé, archiver dans QA.md. À utiliser dès que Donnovan pose une question plutôt qu'il ne demande une tâche.
---

# Répondre aux questions de Donnovan

Lire d'abord [`GOTCHA.md`](GOTCHA.md).

- Citer le code précisément (`engine/platform/src/window.cpp:42`).
- Expliquer le pourquoi avant le comment ; donner le compromis et l'alternative écartée.
- Comparer avec Unreal, Unity et Godot quand c'est pertinent, en distinguant ce qui est **documenté** (avec la
  source) de ce qui est **supposé**. Pour REEngine, Anvil ou Frostbite, ne citer que des sources publiques
  (GDC, CEDEC, blogs techniques).
- Archiver dans `docs/QA.md` toute question dont la réponse mérite d'être retrouvée.
- Pointer vers `docs/LECTURES.md` quand une lecture explique mieux qu'une réponse. Ajouter à ce fichier toute
  bonne source trouvée en chemin, avec ce qu'on y apprend.
- Pour un choix entre deux options (langage, bibliothèque), **montrer du code** et laisser Donnovan choisir à la
  lecture : la méthode a fait ses preuves deux fois (ADR-0009, ADR-0011).
