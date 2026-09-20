# CLAUDE.md — instructions pour Claude Code

## Le projet

**Levain** — moteur de jeu 3D en C++23 sur **NVRHI** (backend Vulkan) et **flecs** (ECS). Namespace racine
`levain`, cibles CMake préfixées `levain_`. **Linux d'abord** : Windows et Direct3D 12 sont différés jusqu'à ce
qu'une machine soit disponible (ADR-0011).
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
- **Forme du code (ADR-0011)**, la règle qui compte le plus pour Donnovan :
  - la logique s'écrit en **fonctions libres dont toutes les dépendances sont des paramètres** ;
  - la **glu ECS tient en une ligne**, jamais dans le fichier de logique ;
  - **chaque piège porte son nom** (`normalizeOrZero`, `clampPitch`) plutôt que d'être un calcul brut.
- Un `README.md` par module : rôle, invariants, points d'entrée, équivalents dans Unreal, Unity et Godot.
- Quand on utilise NVRHI ou flecs d'une manière non évidente, un commentaire renvoie à la section de leur
  documentation qui l'explique.

### À la fin

1. Mesurer les critères du milestone et noter les commandes utilisées.
2. Ouvrir la PR avec le modèle `.github/pull_request_template.md`. Le **guide de lecture** est la partie la plus
   importante : fichiers dans l'ordre, et pour chacun ce qu'il faut y comprendre.
3. Ajouter une entrée à `docs/JOURNAL.md` (format dans le fichier).
4. Mettre à jour le board : Status, et « Passé (h) » dès que Donnovan donne son temps de relecture.
5. Toujours demander à Donnovan, à la fin de la session : « **Combien de temps as-tu passé sur le projet en
   tout ?** » — et non « combien sur la relecture ». Les « Heures Donnovan » de la ROADMAP comptent *tout* son
   engagement : pilotage, questions, décisions, relecture. En ne comptant que la relecture, la phase 0 a été
   sous-évaluée de 3,0 h contre 4,9 h réelles, et le ratio calculé sur cette base aurait déclenché un
   recalibrage injustifié.

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

**Prérequis, une seule fois** : vcpkg cloné et bootstrappé, puis `VCPKG_ROOT` exporté. Les presets lisent cette
variable ; sans elle, `cmake --preset` échoue sur le fichier toolchain.

```bash
git clone https://github.com/microsoft/vcpkg ~/vcpkg && ~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
set -Ux VCPKG_ROOT ~/vcpkg   # fish ; bash : echo 'export VCPKG_ROOT=~/vcpkg' >> ~/.bashrc
```

**Build depuis un clone propre, deux commandes** :

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
```

Le **premier** `cmake --preset` est long : vcpkg compile `nvrhi` et `flecs` depuis les sources. Les suivants
sont instantanés (cache local `~/.cache/vcpkg`).

Presets disponibles : `linux-debug` et `linux-release`, générateur Ninja, Clang. Les presets Windows sont
retirés (ADR-0011) : à remettre avec leur CI le jour où Windows redevient une cible.

```bash
./build/linux-debug/sandbox/levain_sandbox   # lancer la démo
```

`compile_commands.json` est généré dans `build/<preset>/`. Pour clangd à la racine :

```bash
ln -sf build/linux-debug/compile_commands.json compile_commands.json
```

**`VCPKG_ROOT` doit être exportée.** Sans elle, CMake cherche les dépendances dans le système : un build a
déjà trouvé le `spdlog` d'Arch dans `/usr/lib/cmake/spdlog` et continué sans rien dire, contournant la baseline
figée de l'ADR-0007. Le `CMakeLists.txt` racine refuse désormais de se configurer sans la toolchain vcpkg.

**Profilage Tracy**, désactivé par défaut :

```bash
cmake -S . -B build/prof -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DLEVAIN_PROFILING=ON
cmake --build build/prof
TRACY_NO_EXIT=1 ./build/prof/sandbox/levain_sandbox
```

**`TRACY_NO_EXIT=1` n'est pas optionnel** pour un programme court : le sandbox simule 120 frames en ~24 ms,
impossible d'y connecter un profileur à la main. Cette variable fait attendre le client jusqu'à ce que le
profileur se connecte **et** ait reçu toutes les données. Sans elle, le programme se termine avant que quiconque
ait vu quoi que ce soit — et il n'affiche aucun avertissement.

**Ne jamais retirer `-pedantic-errors`** du `CMakeLists.txt` racine : c'est le garde-fou qui maintient le code en
C++23 strict, puisque MSVC compile en `/std:c++latest` (ADR-0001). Une extension C++26 doit casser la CI Linux.

*Tests, format et lint : à compléter en M0.2, issue #5.*
