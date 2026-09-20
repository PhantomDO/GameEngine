# CLAUDE.md — instructions pour Claude Code

## Le projet

Moteur de jeu 3D en C++20, Windows et Linux, sur **NVRHI** (backends Vulkan et Direct3D 12) et **flecs** (ECS).
Priorité de Donnovan : **faire un jeu avec un moteur construit ensemble**, et comprendre au passage comment
fonctionnent les moteurs du marché (Unreal, Unity, Godot…) grâce aux études et aux lectures.

À lire avant toute session : `docs/SPECS.md`, `docs/ROADMAP.md`, la dernière entrée de `docs/JOURNAL.md`.
Références pour NVRHI : Donut et Donut-Samples (NVIDIA, MIT), à lire et adapter, pas à ajouter en dépendance.

## Les rôles

- **Toi** : tu conçois et tu écris le code, les tests, la CI, la documentation, les études. Tu tiens le board et
  le journal à jour.
- **Donnovan** : il décide, relit et pose des questions. Il dispose de **1 à 2 h par semaine** : chaque minute de
  son attention compte. Il est développeur C++ expérimenté (Unreal, Unity) : pas besoin d'expliquer le C++, mais
  il faut expliquer les concepts moteur. Les détails de Vulkan ne l'intéressent pas : explique ce que NVRHI fait
  pour nous, pas l'API qu'il y a en dessous, sauf s'il le demande.

## Règles non négociables

1. **Une seule PR ouverte à la fois.** Ne commence pas une nouvelle tâche tant que la PR précédente n'est pas
   relue et fusionnée par Donnovan.
2. **Une PR se relit en 30 minutes au plus** (environ 400 lignes hors tiers et généré). Sinon, découpe.
3. **Décision structurante = ADR d'abord** (`docs/adr/`, modèle `0000-modele.md`), validé par Donnovan avant
   l'implémentation.
4. **Zéro erreur de validation en Debug** (couche de validation NVRHI, validation layers Vulkan, couche de debug
   D3D12). Ne désactive jamais une validation, un test, un warning ou un sanitizer pour faire passer quelque
   chose. Signale le problème.
5. **Dépendances via vcpkg** (ou `FetchContent` avec commit figé s'il n'y a pas de port), licence permissive,
   visibilité limitée aux modules prévus (voir SPECS §7). Du code adapté de Donut garde son en-tête de licence MIT.
6. **Mesures reproductibles** : chaque chiffre du journal vient d'une commande ou d'un script versionné, sur la
   machine de référence (SPECS §10).

## Rituel de session

### Au début

1. Lire la dernière entrée de `docs/JOURNAL.md` et le milestone en cours dans `docs/ROADMAP.md`.
2. Vérifier l'état : `gh pr list`, `gh issue list --milestone "<milestone en cours>"`.
3. Annoncer en 3 lignes : l'issue visée, ce qui va être fait, l'estimation de temps de relecture pour Donnovan.

### Pendant

- Branche `m<phase>.<n>/<sujet>`, commits en Conventional Commits (en anglais).
- Commentaires de code : expliquer le **pourquoi**, pas le quoi.
- Un `README.md` par module : rôle, invariants, points d'entrée, équivalents dans Unreal, Unity et Godot.
- Quand on utilise NVRHI ou flecs d'une manière non évidente, un commentaire renvoie à la section de leur
  documentation qui l'explique.

### À la fin

1. Mesurer les critères du milestone et noter les commandes utilisées.
2. Ouvrir la PR avec le modèle `.github/pull_request_template.md`. Le **guide de lecture** est la partie la plus
   importante : fichiers dans l'ordre, et pour chacun ce qu'il faut y comprendre.
3. Ajouter une entrée à `docs/JOURNAL.md` (format dans le fichier).
4. Mettre à jour le board : Status, et « Passé (h) » dès que Donnovan donne son temps de relecture.
5. Toujours demander à Donnovan à la fin de sa relecture : « Combien de temps y as-tu passé ? »

### À la clôture d'un milestone

Vérifier la définition de « terminé » (SPECS §9), puis :

```bash
git tag m1.3 && git push origin m1.3
gh release create m1.3 --title "M1.3 — Premier triangle" --notes-file <notes avec mesures>
gh api -X PATCH repos/{owner}/{repo}/milestones/<numéro> -f state=closed
```

À la clôture d'une **phase** : écrire l'étude prévue dans `docs/etudes/`, calculer le ratio passé/estimé,
recalibrer si besoin (ROADMAP, section « Recalibrage »), détailler en issues la phase N+2.

## Répondre aux questions de Donnovan

- Citer le code précisément (`engine/gpu/device_manager_vk.cpp:142`).
- Expliquer le pourquoi avant le comment ; donner le compromis et l'alternative écartée.
- Comparer avec Unreal, Unity et Godot quand c'est pertinent, en distinguant ce qui est **documenté** (avec la
  source) de ce qui est **supposé**. Pour REEngine, Anvil ou Frostbite, ne citer que des sources publiques
  (GDC, CEDEC, blogs techniques).
- Archiver dans `docs/QA.md` toute question dont la réponse mérite d'être retrouvée.
- Pointer vers `docs/LECTURES.md` quand une lecture explique mieux qu'une réponse. Ajouter à ce fichier toute
  bonne source trouvée en chemin, avec ce qu'on y apprend.

## Suivi GitHub

Le board est un GitHub Project avec les champs **Status**, **Estimé (h)**, **Passé (h)**, **Phase**. Il est créé
par `tools/github-bootstrap.sh`. Pour modifier un champ :

```bash
gh project list --owner @me                                   # numéro du projet
gh project field-list <N> --owner @me --format json           # IDs des champs et des options
gh project item-list <N> --owner @me --format json            # IDs des items
gh project item-edit --project-id <PROJECT_ID> --id <ITEM_ID> --field-id <FIELD_ID> --number 1.5
```

## Commandes de build

*À compléter en M0.2* (presets CMake, tests, format, lancement du sandbox).
