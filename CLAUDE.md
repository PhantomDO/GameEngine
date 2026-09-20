# CLAUDE.md — instructions pour Claude Code

## Le projet

**Levain** — moteur de jeu 3D en **Rust** (edition 2024), Windows et Linux, sur **wgpu** (backends Vulkan,
Direct3D 12, Metal) et **bevy_ecs**. Crates préfixés `levain-`, workspace cargo à la racine.
Priorité de Donnovan : **faire un jeu avec un moteur construit ensemble**, et comprendre au passage comment
fonctionnent les moteurs du marché (Unreal, Unity, Godot…) grâce aux études et aux lectures.

À lire avant toute session : `docs/SPECS.md`, `docs/ROADMAP.md`, la dernière entrée de `docs/JOURNAL.md`.
Références de lecture : **Bevy** (notamment `bevy_render`) et les **exemples wgpu**, à lire et adapter, pas à
ajouter en dépendance. Le parcours Donut de la v0.3 est caduc depuis l'ADR-0010.

## Les rôles

- **Toi** : tu conçois et tu écris le code, les tests, la CI, la documentation, les études. Tu tiens le board et
  le journal à jour.
- **Donnovan** : il décide, relit et pose des questions. Il dispose de **1 à 2 h par semaine** : chaque minute de
  son attention compte. Il est développeur C++ expérimenté (Unreal, Unity) et **débutant en Rust** : les
  concepts moteur et les idiomes Rust méritent une explication, la programmation générale non. Les détails de
  Vulkan ne l'intéressent pas : explique ce que wgpu fait pour nous, pas l'API en dessous, sauf demande.

## Règles non négociables

1. **Une seule PR ouverte à la fois.** Ne commence pas une nouvelle tâche tant que la PR précédente n'est pas
   relue et fusionnée par Donnovan.
2. **Une PR se relit en 30 minutes au plus** (environ 400 lignes hors tiers et généré). Sinon, découpe.
3. **Décision structurante = ADR d'abord** (`docs/adr/`, modèle `0000-modele.md`), validé par Donnovan avant
   l'implémentation.
4. **Zéro erreur de validation en Debug** (couche de validation NVRHI, validation layers Vulkan, couche de debug
   D3D12). Ne désactive jamais une validation, un test, un warning ou un sanitizer pour faire passer quelque
   chose. Signale le problème.
5. **Dépendances via cargo**, licence permissive, visibilité limitée aux modules prévus (voir SPECS §7).
   `unsafe_code = "deny"` au niveau du workspace : toute levée est locale, commentée et justifiée. Du code adapté
   de Bevy ou des exemples wgpu garde son en-tête de licence.
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
- Quand on utilise wgpu ou bevy_ecs d'une manière non évidente, un commentaire renvoie à la section de leur
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

- Citer le code précisément (`engine/gpu/src/device.rs:142`).
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

**Prérequis, une seule fois** : `rustup` installé (`curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`).
Aucune autre dépendance système : c'est tout l'intérêt de l'ADR-0010.

```bash
cargo build --workspace          # compiler
cargo run -p levain-sandbox      # lancer la démo
cargo test --workspace           # tests
cargo fmt --all                  # formater
cargo clippy --workspace --all-targets -- -D warnings   # lint, comme en CI
```

Profils : `dev` par défaut, `--release` pour les mesures. Les jobs de CI s'appellent `linux-debug`,
`linux-release`, `windows-debug`, `windows-release` — **ne pas les renommer** : ce sont les checks requis par la
protection de `main`, et les changer rendrait la branche infusionnable.

**Ne jamais lever `unsafe_code = "deny"` globalement.** Les frontières GPU qui en auront besoin le font
localement, avec un commentaire de sûreté, pour rester repérables par un `grep`.
