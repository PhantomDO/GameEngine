#!/usr/bin/env bash
# Crée le suivi GitHub du moteur : dépôt public, labels, 35 milestones (avec échéances),
# board GitHub Projects (champs Estimé, Passé, Phase) et issues des phases 0 et 1.
#
# Usage :
#   ./tools/github-bootstrap.sh <nom-du-depot>             # crée tout
#   DRY_RUN=1 ./tools/github-bootstrap.sh <nom-du-depot>   # affiche sans rien créer
#
# Prérequis : gh authentifié avec le scope "project" (gh auth refresh -s project),
# à lancer depuis la racine du dépôt git local, après un premier commit.
# Relançable : ce qui existe déjà (dépôt, labels, milestones, board, champs, issues) est sauté.

set -euo pipefail

NAME="${1:?Usage : $0 <nom-du-depot>}"
DRY_RUN="${DRY_RUN:-0}"
DESCRIPTION="Moteur de jeu 3D en C++20 (NVRHI, flecs), construit étape par étape pour faire un jeu et comprendre les moteurs du marché"
PROJECT_TITLE="${NAME} — Roadmap"

log()  { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
skip() { printf '    (existe déjà) %s\n' "$*"; }
warn() { printf '\033[1;33m[!]\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31m[x]\033[0m %s\n' "$*" >&2; exit 1; }
is_dry() { [[ "$DRY_RUN" == "1" ]]; }

# Exécute une commande qui modifie quelque chose ; en DRY_RUN, l'affiche seulement.
mutate() {
  if is_dry; then
    local out="    [dry-run]" x
    for x in "$@"; do
      if [[ "$x" =~ [[:space:]] ]]; then out+=" \"$x\""; else out+=" $x"; fi
    done
    printf '%s\n' "$out"
  else
    "$@"
  fi
}

# ---------------------------------------------------------------------------
# 0. Prérequis
# ---------------------------------------------------------------------------
command -v gh  >/dev/null || die "gh (GitHub CLI) introuvable."
command -v git >/dev/null || die "git introuvable."
gh auth status >/dev/null 2>&1 || die "gh n'est pas authentifié : lance 'gh auth login'."
git rev-parse --verify HEAD >/dev/null 2>&1 || die "Aucun commit : fais 'git init -b main' puis un premier commit."

# ---------------------------------------------------------------------------
# 1. Dépôt
# ---------------------------------------------------------------------------
REPO_EXISTS=1
if git remote get-url origin >/dev/null 2>&1; then
  REPO_FULL="$(gh repo view --json nameWithOwner --jq .nameWithOwner)"
  log "Dépôt existant : $REPO_FULL"
else
  LOGIN="$(gh api user --jq .login)"
  REPO_FULL="${LOGIN}/${NAME}"
  log "Création du dépôt public $REPO_FULL"
  mutate gh repo create "$NAME" --public --description "$DESCRIPTION" --source . --remote origin --push
  if is_dry; then REPO_EXISTS=0; fi
fi
OWNER="${REPO_FULL%%/*}"

# ---------------------------------------------------------------------------
# 2. Labels (--force met à jour un label existant)
# ---------------------------------------------------------------------------
log "Labels"
while IFS='|' read -r label color desc; do
  [[ -z "$label" ]] && continue
  mutate gh label create "$label" --repo "$REPO_FULL" --color "$color" --description "$desc" --force
done <<'LABELS'
type:feature|0e8a16|Fonctionnalité du moteur
type:infra|c5def5|Build, CI, outillage
type:decision|5319e7|Décision à prendre ou à valider (ADR)
type:etude|1d76db|Étude comparative (Unreal, Unity, Godot…)
type:question|d876e3|Question de Donnovan à approfondir
type:bug|d73a4a|Bug
area:core|ededed|Module core
area:platform|ededed|Module platform (SDL3)
area:gpu|ededed|Module gpu (DeviceManager, NVRHI)
area:render|ededed|Module render
area:scene|ededed|Module scene (flecs)
area:assets|ededed|Module assets
area:physics|ededed|Module physics (Jolt)
area:audio|ededed|Module audio
area:editor|ededed|Éditeur
area:build|ededed|CMake, vcpkg, CI
area:docs|ededed|Specs, ADR, études, journal
LABELS

# ---------------------------------------------------------------------------
# 3. Milestones : un par milestone de la roadmap, échéance à 1,5 h/semaine
# ---------------------------------------------------------------------------
log "Milestones"
EXISTING_MS=""
if [[ "$REPO_EXISTS" == 1 ]]; then
  EXISTING_MS="$(gh api "repos/${REPO_FULL}/milestones?state=all&per_page=100" --jq '.[].title')"
fi
while IFS='|' read -r title due hours sessions; do
  [[ -z "$title" ]] && continue
  if grep -Fxq -- "$title" <<<"$EXISTING_MS"; then skip "$title"; continue; fi
  mutate gh api --method POST "repos/${REPO_FULL}/milestones" \
    -f title="$title" \
    -f due_on="${due}T12:00:00Z" \
    -f description="Estimé : ${hours} h Donnovan, ${sessions} session(s) Claude Code. Détails : docs/ROADMAP.md" \
    --silent
done <<'MILESTONES'
M0.1 — Dépôt, suivi et specs|2026-09-27|1.0|0
M0.2 — Squelette de build et CI|2026-10-04|1.5|2
M0.3 — Core minimal|2026-10-11|1.5|1
M1.1 — Fenêtre et boucle|2026-10-18|1.5|1
M1.2 — Device NVRHI (Vulkan) et swapchain|2026-10-25|1.5|1
M1.3 — Premier triangle|2026-11-01|1.5|1
M1.4 — Backend Direct3D 12|2026-11-08|1.0|1
M2.1 — Caméra, meshes et binding sets|2026-11-15|2.5|2
M2.2 — Textures|2026-11-22|1.5|1
M2.3 — Hot-reload des shaders|2026-11-29|1.5|1
M3.1 — Intégration de flecs et explorer|2026-12-06|1.5|1
M3.2 — Transforms et hiérarchie|2026-12-13|1.0|1
M3.3 — Boucle à pas fixe|2026-12-20|1.5|1
M3.4 — Input par actions et caméra libre|2026-12-27|1.5|1
M3.5 — Choix du jeu|2026-12-27|0.5|0
M4.1 — Import glTF|2027-01-10|2.0|2
M4.2 — Base d'assets|2027-01-17|2.5|2
M4.3 — Cuisson des assets|2027-01-31|3.0|2
M4.4 — Hot-reload des assets|2027-02-07|1.5|1
M5.1 — PBR direct|2027-02-21|2.5|2
M5.2 — HDR et tonemapping|2027-02-28|1.5|1
M5.3 — Ombres en cascades|2027-03-14|2.5|2
M5.4 — Éclairage d'environnement (IBL)|2027-03-21|2.5|2
M5.5 — Culling et statistiques|2027-04-04|2.0|1
M6.1 — Intégration Jolt|2027-04-11|2.5|2
M6.2 — Colliders, requêtes, debug draw|2027-04-25|2.0|1
M6.3 — Character controller|2027-05-02|2.0|1
M7.1 — ImGui et panneaux de debug|2027-05-09|1.5|1
M7.2 — Réflexion et inspecteur|2027-05-16|2.0|1
M7.3 — Sérialisation et undo/redo|2027-05-30|2.0|2
M7.4 — Gizmos et picking|2027-06-06|2.5|2
M7.5 — Play/Stop dans l'éditeur|2027-06-13|1.5|1
M8.1 — Audio|2027-06-20|1.5|1
M8.2 — Le jeu (vertical slice)|2027-08-01|8.0|6
M8.3 — Bilan v1|2027-08-08|1.5|1
MILESTONES

# ---------------------------------------------------------------------------
# 4. Board GitHub Projects et champs personnalisés
# ---------------------------------------------------------------------------
log "Board : $PROJECT_TITLE"
PROJECT_EXISTS=1
PNUM="$(gh project list --owner "$OWNER" --limit 100 --format json \
        --jq ".projects[] | select(.title == \"${PROJECT_TITLE}\") | .number" | awk 'NR == 1')"
if [[ -n "$PNUM" ]]; then
  skip "board n°$PNUM"
else
  if is_dry; then
    mutate gh project create --owner "$OWNER" --title "$PROJECT_TITLE"
    PROJECT_EXISTS=0; PNUM="<N>"
  else
    PNUM="$(gh project create --owner "$OWNER" --title "$PROJECT_TITLE" --format json --jq .number)"
    log "Board créé : n°$PNUM"
  fi
  mutate gh project link "$PNUM" --owner "$OWNER" --repo "$REPO_FULL" \
    || warn "Liaison board ↔ dépôt impossible : à faire à la main dans l'onglet Projects du dépôt."
fi

EXISTING_FIELDS=""
if [[ "$PROJECT_EXISTS" == 1 ]]; then
  EXISTING_FIELDS="$(gh project field-list "$PNUM" --owner "$OWNER" --limit 100 --format json --jq '.fields[].name')"
fi
create_field() { # $1 = nom, reste = options de gh project field-create
  local field="$1"; shift
  if grep -Fxq -- "$field" <<<"$EXISTING_FIELDS"; then skip "champ $field"; return; fi
  mutate gh project field-create "$PNUM" --owner "$OWNER" --name "$field" "$@"
}
create_field "Estimé (h)" --data-type NUMBER
create_field "Passé (h)"  --data-type NUMBER
create_field "Phase"      --data-type SINGLE_SELECT --single-select-options "P0,P1,P2,P3,P4,P5,P6,P7,P8"

if [[ "$PROJECT_EXISTS" == 1 ]]; then
  PROJECT_ID="$(gh project view "$PNUM" --owner "$OWNER" --format json --jq .id)"
  EST_FIELD="$(gh project field-list "$PNUM" --owner "$OWNER" --limit 100 --format json \
               --jq '.fields[] | select(.name == "Estimé (h)") | .id')"
  PHASE_FIELD="$(gh project field-list "$PNUM" --owner "$OWNER" --limit 100 --format json \
                 --jq '.fields[] | select(.name == "Phase") | .id')"
  PHASE_OPTIONS="$(gh project field-list "$PNUM" --owner "$OWNER" --limit 100 --format json \
                   --jq '.fields[] | select(.name == "Phase") | .options[] | "\(.name) \(.id)"')"
fi

# ---------------------------------------------------------------------------
# 5. Issues des phases 0 et 1 (les suivantes sont détaillées à l'approche)
# ---------------------------------------------------------------------------
log "Issues des phases 0 et 1"
EXISTING_ISSUES=""
if [[ "$REPO_EXISTS" == 1 ]]; then
  EXISTING_ISSUES="$(gh issue list --repo "$REPO_FULL" --state all --limit 500 --json title --jq '.[].title')"
fi

# issue <titre> <milestone> <phase> <heures> <labels> ; corps lu sur l'entrée standard
issue() {
  local title="$1" milestone="$2" phase="$3" hours="$4" labels="$5" body
  body="$(cat)"
  body+=$'\n\n## Estimation\n\n- Heures Donnovan : '"$hours"$'\n- Milestone : '"$milestone"
  if grep -Fxq -- "$title" <<<"$EXISTING_ISSUES"; then skip "$title"; return; fi
  if is_dry; then
    printf '    [dry-run] issue « %s » (%s, %s h, %s)\n' "$title" "${milestone%% —*}" "$hours" "$labels"
    return
  fi
  local url item option
  option="$(awk -v o="$phase" '$1 == o { print $2 }' <<<"$PHASE_OPTIONS")"
  url="$(gh issue create --repo "$REPO_FULL" --title "$title" --body "$body" \
          --milestone "$milestone" --label "$labels")"
  item="$(gh project item-add "$PNUM" --owner "$OWNER" --url "$url" --format json --jq .id)"
  gh project item-edit --project-id "$PROJECT_ID" --id "$item" --field-id "$EST_FIELD" --number "$hours" >/dev/null
  gh project item-edit --project-id "$PROJECT_ID" --id "$item" --field-id "$PHASE_FIELD" \
    --single-select-option-id "$option" >/dev/null
  printf '    %s\n' "$url"
}

M01="M0.1 — Dépôt, suivi et specs"
M02="M0.2 — Squelette de build et CI"
M03="M0.3 — Core minimal"
M11="M1.1 — Fenêtre et boucle"
M12="M1.2 — Device NVRHI (Vulkan) et swapchain"
M13="M1.3 — Premier triangle"
M14="M1.4 — Backend Direct3D 12"

issue "Valider SPECS, ROADMAP et ADR 0001 à 0007" "$M01" P0 0.75 "type:decision,area:docs" <<'B'
## Objectif
Relire `docs/SPECS.md`, `docs/ROADMAP.md` et `docs/adr/0001` à `0007` ; amender ou accepter. Les ADR acceptés
passent au statut « accepté ».

## Critères
- [ ] Chaque ADR a le statut « accepté » ou a été amendé
- [ ] Les estimations de la roadmap sont jugées réalistes
B

issue "Renseigner la machine de référence, le nom et la licence" "$M01" P0 0.25 "type:decision,area:docs" <<'B'
## Objectif
Compléter SPECS §10 (OS, CPU, GPU, pilote, RAM) et les questions ouvertes de SPECS §11 (nom, licence, accès
à Windows).

## Critères
- [ ] Tableau de la machine de référence rempli (sortie de `vulkaninfo --summary` jointe)
- [ ] Nom et licence choisis
B

issue "Arborescence, CMake presets et vcpkg en mode manifeste" "$M02" P0 0.5 "type:infra,area:build" <<'B'
## Objectif
Créer l'arborescence de SPECS §7, `CMakePresets.json` (linux-debug, linux-release, windows-debug,
windows-release, Ninja), `vcpkg.json` avec une baseline figée (dont nvrhi et flecs), et un exécutable `sandbox`
qui affiche une ligne.

## Critères
- [ ] Build depuis un clone propre en deux commandes sur Linux et sur Windows
- [ ] `compile_commands.json` généré
- [ ] Section « Commandes de build » de CLAUDE.md complétée

## Concepts à comprendre
Presets CMake, mode manifeste de vcpkg, triplets, baseline. Comparaison : UBT d'Unreal, SCons de Godot.
B

issue "CI GitHub Actions Windows + Linux avec cache vcpkg" "$M02" P0 0.5 "type:infra,area:build" <<'B'
## Objectif
Workflow qui compile et lance les tests en Debug et Release sur `ubuntu-latest` et `windows-latest`, avec cache
binaire vcpkg.

## Critères
- [ ] CI verte sur les deux OS
- [ ] Durée de CI à froid et à chaud mesurée et notée dans le journal
B

issue "clang-format, clang-tidy, doctest et protection de main" "$M02" P0 0.5 "type:infra,area:build" <<'B'
## Objectif
`.clang-format` et `.clang-tidy` versionnés et vérifiés en CI ; doctest intégré avec un premier test ;
protection de la branche `main` (PR obligatoire, CI verte requise).

## Critères
- [ ] Un code mal formaté fait échouer la CI
- [ ] `ctest` lance au moins un test
- [ ] Push direct sur `main` refusé
B

issue "Logs, assertions et gestion d'erreurs (ADR-0008)" "$M03" P0 0.5 "type:feature,area:core" <<'B'
## Objectif
Logs par catégorie (spdlog), assertions actives en Debug, et ADR-0008 sur la gestion d'erreurs (exceptions ou
codes de retour).

## Critères
- [ ] ADR-0008 validé par Donnovan
- [ ] Une assertion qui échoue affiche fichier, ligne et message, puis s'arrête dans le débogueur

## Concepts à comprendre
Pourquoi la plupart des moteurs désactivent les exceptions ; `check`/`ensure`/`verify` d'Unreal.
B

issue "Allocateurs linéaire et pool, avec benchmarks" "$M03" P0 0.5 "type:feature,area:core" <<'B'
## Objectif
Allocateur linéaire (remis à zéro à chaque frame) et allocateur pool (objets de taille fixe), avec tests
unitaires et benchmark contre `malloc`.

## Critères
- [ ] Tests unitaires verts (alignement, débordement, remise à zéro)
- [ ] Benchmark chiffré dans le journal

## Concepts à comprendre
Pourquoi les moteurs évitent l'allocateur général dans la boucle de jeu ; `FMemStack` d'Unreal.
B

issue "Intégration de Tracy" "$M03" P0 0.25 "type:feature,area:core" <<'B'
## Objectif
Intégrer le client Tracy (désactivable à la compilation) et instrumenter la boucle.

## Critères
- [ ] Zones visibles dans une capture Tracy (capture d'écran dans la PR)
- [ ] Coût nul quand Tracy est désactivé
B

issue "Étude E0 : comment démarre un moteur" "$M03" P0 0.25 "type:etude,area:docs" <<'B'
## Objectif
Écrire `docs/etudes/E0-demarrage.md` : `FEngineLoop` d'Unreal (PreInit, Init, Tick), PlayerLoop d'Unity,
`Main::setup` et `Main::iteration` de Godot, et ce que fait notre moteur.
B

issue "Fenêtre SDL3, boucle principale et événements" "$M11" P1 0.75 "type:feature,area:platform" <<'B'
## Objectif
Ouvrir une fenêtre SDL3 (X11 et Wayland sous Linux), boucle principale, événements de fermeture, de
redimensionnement et de minimisation, traduits vers nos propres types.

## Critères
- [ ] Redimensionnement et minimisation sans plantage
- [ ] Aucun en-tête SDL visible hors de `engine/platform/` (sauf création de surface dans `engine/gpu/`)
B

issue "Mesure du frame time et sanitizers en CI Linux" "$M11" P1 0.75 "type:infra,area:build" <<'B'
## Objectif
Afficher le frame time (moyenne, min, max sur 1 s) ; ajouter une configuration CI Linux avec ASan et UBSan.

## Critères
- [ ] Frame time affiché dans le titre de la fenêtre
- [ ] Job sanitizers vert, zéro fuite signalée
B

issue "DeviceManager Vulkan pour NVRHI (d'après Donut)" "$M12" P1 1.0 "type:feature,area:gpu" <<'B'
## Objectif
Créer instance, device et queues Vulkan (en s'inspirant de `DeviceManager_VK` de Donut, adapté à SDL3 ; vk-bootstrap
si ça simplifie), puis `nvrhi::vulkan::createDevice`. Activer la couche de validation NVRHI et les validation
layers Vulkan en Debug, messages redirigés dans nos logs.

## Critères
- [ ] Nom du GPU et version du pilote affichés au démarrage
- [ ] Zéro message de validation au lancement

## Concepts à comprendre
Ce que NVRHI attend de l'application (device, queues, extensions) et ce qu'il prend en charge ensuite.
Lecture : étude E1 et `doc/ProgrammingGuide.md` de NVRHI.
B

issue "Swapchain, redimensionnement et écran effacé via NVRHI" "$M12" P1 0.5 "type:feature,area:gpu" <<'B'
## Objectif
Swapchain Vulkan exposée à NVRHI, recréée au redimensionnement ; chaque frame efface l'écran à une couleur via
une command list NVRHI.

## Critères
- [ ] Zéro erreur de validation sur 5 minutes avec redimensionnements
- [ ] Temps de démarrage mesuré
B

issue "Compilation des shaders Slang en SPIR-V et DXIL" "$M13" P1 0.5 "type:infra,area:render" <<'B'
## Objectif
Compiler `shaders/*.slang` au build vers SPIR-V et DXIL, en respectant la convention de binding de NVRHI
(décalages Vulkan). Choisir entre ShaderMake et des commandes CMake, noter le choix dans ADR-0005.

## Critères
- [ ] Une erreur de shader fait échouer le build avec un message lisible
- [ ] Un shader modifié est recompilé, les autres non
- [ ] Fonctionne en CI sur les deux OS
B

issue "Premier triangle avec NVRHI" "$M13" P1 0.5 "type:feature,area:render" <<'B'
## Objectif
Pipeline graphique, framebuffer et command list NVRHI pour dessiner un triangle (modèle : Basic Triangle de
Donut-Samples).

## Critères
- [ ] Triangle affiché sous Linux
- [ ] Zéro erreur de validation
- [ ] Frame time CPU < 1 ms
B

issue "Test de fumée headless sous lavapipe en CI Linux" "$M13" P1 0.25 "type:infra,area:build" <<'B'
## Objectif
Rendre le triangle hors écran sous lavapipe (Vulkan logiciel de Mesa) en CI, lire l'image et la comparer à
une image de référence.

## Critères
- [ ] Le test échoue si le triangle ne s'affiche plus
B

issue "Lire l'étude E1 et les lectures NVRHI" "$M13" P1 0.25 "type:etude,area:docs" <<'B'
## Objectif
Lire `docs/etudes/E1-rhi.md` et la section NVRHI de `docs/LECTURES.md`. Noter les questions dans `docs/QA.md`
ou en issues `type:question`.
B

issue "DeviceManager Direct3D 12 et choix du backend au lancement" "$M14" P1 0.75 "type:feature,area:gpu" <<'B'
## Objectif
`DeviceManager` D3D12 (d'après `DeviceManager_DX12` de Donut), `nvrhi::d3d12::createDevice`, couche de debug
D3D12 ; option `--api vulkan|d3d12`.

## Critères
- [ ] Le même triangle sous les deux backends, D3D12 vérifié en lançant le binaire de la CI sous Proton
      (voir SPECS §10 et SETUP §4)
- [ ] Zéro erreur de la couche de debug D3D12 et de la validation NVRHI
B

issue "Test de fumée D3D12 sous WARP en CI Windows" "$M14" P1 0.25 "type:infra,area:build" <<'B'
## Objectif
Rendre le triangle hors écran avec l'adaptateur WARP (D3D12 logiciel) sur le runner Windows et comparer l'image
à la référence.

## Critères
- [ ] Le test échoue si le triangle ne s'affiche plus sous D3D12
B

log "Terminé. Board : https://github.com/users/${OWNER}/projects/${PNUM}"
